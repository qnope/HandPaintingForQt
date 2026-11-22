// DIAGNOSTIC (temporary): taps the application like a phone does and reports what happens to the
// focus of the page and to the text typed afterwards.
const fs = require('fs');
const http = require('http');
const path = require('path');
const { chromium, devices } = require('playwright');

const ContentTypes = {
    '.html': 'text/html; charset=utf-8',
    '.js': 'text/javascript',
    '.wasm': 'application/wasm',
    '.ttf': 'font/ttf',
};

function serve(site_directory)
{
    const server = http.createServer((request, response) => {
        const url_path = decodeURIComponent(new URL(request.url, 'http://localhost').pathname);
        const file_path = path.join(site_directory, url_path.endsWith('/') ? url_path + 'index.html' : url_path);
        if (!file_path.startsWith(site_directory) || !fs.existsSync(file_path) || !fs.statSync(file_path).isFile()) {
            response.writeHead(404).end('Not found');
            return;
        }
        response.writeHead(200, { 'Content-Type': ContentTypes[path.extname(file_path)] ?? 'application/octet-stream' });
        fs.createReadStream(file_path).pipe(response);
    });
    return new Promise(resolve => server.listen(0, '127.0.0.1', () => resolve(server)));
}

function install_probes()
{
    const describe = element => {
        if (!element)
            return 'null';
        let text = element.tagName;
        if (element.id)
            text += '#' + element.id;
        if (element.className && typeof element.className === 'string')
            text += '.' + element.className.replace(/ /g, '.');
        return text;
    };
    window.hpq_describe_active = () => {
        let active = document.activeElement;
        const chain = [describe(active)];
        while (active && active.shadowRoot && active.shadowRoot.activeElement) {
            active = active.shadowRoot.activeElement;
            chain.push(describe(active));
        }
        return chain.join(' > ');
    };
    window.hpq_input = () => document.querySelector('body > input');
    window.hpq_input_state = () => {
        const input = window.hpq_input();
        if (!input)
            return 'no input element';
        const rect = input.getBoundingClientRect();
        return JSON.stringify({ value: input.value, focused: document.activeElement === input,
                                inputMode: input.inputMode, left: rect.left, top: rect.top,
                                width: rect.width, height: rect.height,
                                selection: [input.selectionStart, input.selectionEnd] });
    };
    window.hpq_verbose = false;
    for (const type of ['focusin', 'focusout', 'pointerdown', 'pointerup', 'touchstart', 'touchend',
                        'mousedown', 'mouseup', 'click', 'keydown', 'keyup', 'beforeinput', 'input',
                        'compositionstart', 'compositionupdate', 'compositionend']) {
        window.addEventListener(type, event => {
            if (!window.hpq_verbose)
                return;
            const target = event.composedPath()[0];
            let details = '';
            if (type.startsWith('key'))
                details = ` key=${event.key} code=${event.keyCode} composing=${event.isComposing}`;
            if (type.endsWith('input'))
                details = ` inputType=${event.inputType} data=${event.data}`;
            if (type.startsWith('composition'))
                details = ` data=${event.data}`;
            if (type.startsWith('pointer'))
                details = ` pointerType=${event.pointerType}`;
            console.log(`[event] ${type} on ${describe(target)}${details} prevented=${event.defaultPrevented}`);
        }, true);
    }
}

async function main()
{
    const site_directory = path.resolve(process.argv[2] ?? '');
    const device_name = process.argv[3] ?? 'Pixel 7';
    const server = await serve(site_directory);
    const browser = await chromium.launch(process.env.CHROME_BIN ? { executablePath: process.env.CHROME_BIN } : {});
    try {
        const context = await browser.newContext({ ...devices[device_name] });
        const page = await context.newPage();
        page.on('console', message => console.log(`[browser ${message.type()}] ${message.text()}`));
        page.on('pageerror', error => console.log(`[pageerror] ${error.message}`));
        await page.addInitScript(install_probes);
        await page.goto(`http://127.0.0.1:${server.address().port}/`);
        await page.waitForFunction(() => document.documentElement.dataset.state === 'running', undefined, { timeout: 120000 });
        await page.waitForTimeout(3000);

        const viewport = page.viewportSize();
        console.log(`Device: ${device_name}, viewport ${JSON.stringify(viewport)}`);
        console.log(`Active at start: ${await page.evaluate(() => window.hpq_describe_active())}`);
        console.log(`Input at start: ${await page.evaluate(() => window.hpq_input_state())}`);
        console.log(`Screen rect: ${await page.evaluate(() => JSON.stringify(document.querySelector('#screen').getBoundingClientRect()))}`);

        // Scan upwards, at 60 % of the width: which taps give the focus to the input element of Qt?
        const x = Math.round(viewport.width * 0.6);
        const hits = [];
        for (let y = Math.round(viewport.height * 0.45); y > 60; y -= 4) {
            await page.touchscreen.tap(x, y);
            await page.waitForTimeout(60);
            const active = await page.evaluate(() => window.hpq_describe_active());
            const is_input = active.startsWith('INPUT');
            hits.push({ y, is_input });
            console.log(`tap (${x}, ${y}): active=${active} input=${await page.evaluate(() => window.hpq_input_state())}`);
            // Stop after the second band of hits: the first is the font combo box, the second the line edit.
            const bands = hits.filter((hit, index) => hit.is_input && (index === 0 || !hits[index - 1].is_input)).length;
            if (bands >= 2 && !is_input)
                break;
        }
        const band_starts = hits.filter((hit, index) => hit.is_input && (index === 0 || !hits[index - 1].is_input));
        console.log(`Bands start at: ${JSON.stringify(band_starts)}`);
        if (band_starts.length === 0) {
            console.log('RESULT: no tap gave the focus to the input element of Qt.');
            return 0;
        }
        const last_band = hits.filter(hit => hit.is_input && hit.y <= band_starts[band_starts.length - 1].y);
        const line_edit_y = Math.round((last_band[0].y + last_band[last_band.length - 1].y) / 2);
        console.log(`Line edit taken at y=${line_edit_y}`);

        await page.evaluate(() => { window.hpq_verbose = true; });
        console.log('--- tap on the line edit');
        await page.touchscreen.tap(x, line_edit_y);
        await page.waitForTimeout(500);
        console.log(`active=${await page.evaluate(() => window.hpq_describe_active())} input=${await page.evaluate(() => window.hpq_input_state())}`);

        console.log('--- keyboard.type("ab")');
        await page.keyboard.type('ab', { delay: 50 });
        await page.waitForTimeout(300);
        console.log(`active=${await page.evaluate(() => window.hpq_describe_active())} input=${await page.evaluate(() => window.hpq_input_state())}`);

        console.log('--- composition "cd" then commit (as a phone keyboard does)');
        const session = await context.newCDPSession(page);
        await session.send('Input.imeSetComposition', { text: 'c', selectionStart: 1, selectionEnd: 1 });
        await page.waitForTimeout(100);
        await session.send('Input.imeSetComposition', { text: 'cd', selectionStart: 2, selectionEnd: 2 });
        await page.waitForTimeout(100);
        await session.send('Input.insertText', { text: 'cd' });
        await page.waitForTimeout(300);
        console.log(`active=${await page.evaluate(() => window.hpq_describe_active())} input=${await page.evaluate(() => window.hpq_input_state())}`);

        console.log('--- tap again on the line edit: Qt copies the text of the widget in the input element');
        await page.touchscreen.tap(x, line_edit_y);
        await page.waitForTimeout(500);
        console.log(`active=${await page.evaluate(() => window.hpq_describe_active())} input=${await page.evaluate(() => window.hpq_input_state())}`);
        console.log(`RESULT: text of the line edit read back = ${await page.evaluate(() => window.hpq_input().value)}`);
    } finally {
        await browser.close();
        server.close();
    }
    return 0;
}

main().then(code => process.exit(code), error => {
    console.error(error);
    process.exit(1);
});
