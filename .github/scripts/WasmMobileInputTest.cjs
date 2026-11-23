// DIAGNOSTIC (temporary): taps the application like a phone does and reports what happens to the
// focus of the page and to the text typed afterwards.
const fs = require('fs');
const http = require('http');
const path = require('path');
const playwright = require('playwright');

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
    window.hpq_state = () => {
        let active = document.activeElement;
        const chain = [describe(active)];
        while (active && active.shadowRoot && active.shadowRoot.activeElement) {
            active = active.shadowRoot.activeElement;
            chain.push(describe(active));
        }
        const input = document.querySelector('body > input');
        const rect = input ? input.getBoundingClientRect() : {};
        return `active=${chain.join('>')} input.value=${JSON.stringify(input?.value)} at (${rect.left}, ${rect.top})`;
    };
    window.hpq_cursor = () => {
        const find = root => {
            for (const element of root.querySelectorAll('*')) {
                if (element.classList.contains('qt-window-contents'))
                    return element;
                const found = element.shadowRoot ? find(element.shadowRoot) : null;
                if (found)
                    return found;
            }
            return null;
        };
        return find(document)?.style.cursor ?? 'none found';
    };
    window.hpq_verbose = false;
    for (const type of ['focusin', 'focusout', 'pointerdown', 'pointerup', 'pointercancel', 'touchstart', 'touchend',
                        'mousedown', 'mouseup', 'click', 'keydown', 'keyup', 'input',
                        'compositionstart', 'compositionupdate', 'compositionend']) {
        window.addEventListener(type, event => {
            if (!window.hpq_verbose)
                return;
            const target = event.composedPath()[0];
            let details = '';
            if (type.startsWith('key'))
                details = ` key=${event.key} code=${event.keyCode} composing=${event.isComposing}`;
            if (type === 'input')
                details = ` inputType=${event.inputType} data=${event.data}`;
            if (type.startsWith('composition'))
                details = ` data=${event.data}`;
            if (type.startsWith('pointer'))
                details = ` pointerType=${event.pointerType}`;
            console.log(`[event] ${type} on ${describe(target)}${details}`);
        }, true);
    }
}

// Candidate fix, as the page would install it.
function install_fix()
{
    document.querySelector('#screen').addEventListener('touchend', event => {
        const in_window_contents = event.composedPath().some(
            element => element.classList && element.classList.contains('qt-window-contents'));
        if (in_window_contents && event.cancelable)
            event.preventDefault();
    }, { capture: true, passive: false });
}

const QtLogFilter = /setFocusObject|showInputPanel|hideInputPanel|insertPreedit|commitPreedit|Key callback|processKey|inputType/;

async function run(site_url, browser_name, device_name, with_fix)
{
    const use_chrome = browser_name === 'chromium' && process.env.CHROME_BIN;
    const browser = await playwright[browser_name].launch(use_chrome ? { executablePath: process.env.CHROME_BIN } : {});
    console.log(`\n===== ${browser_name} ${browser.version()}, ${device_name}, ${with_fix ? 'WITH' : 'without'} the fix`);
    try {
        const context = await browser.newContext({ ...playwright.devices[device_name] });
        const page = await context.newPage();
        let show_console = false;
        page.on('console', message => {
            const text = message.text().trim();
            if (show_console && text && (text.startsWith('[event]') || QtLogFilter.test(text)))
                console.log(`    ${text.replace('qt.qpa.wasm.inputcontext: ', 'Qt: ')}`);
        });
        page.on('pageerror', error => console.log(`[pageerror] ${error.message}`));
        await page.addInitScript(install_probes);
        await page.goto(site_url);
        await page.waitForFunction(() => document.documentElement.dataset.state === 'running', undefined, { timeout: 120000 });
        await page.waitForTimeout(3000);
        if (with_fix)
            await page.evaluate(install_fix);

        const viewport = page.viewportSize();
        console.log(`viewport ${JSON.stringify(viewport)}; ${await page.evaluate(() => window.hpq_state())}`);

        // The mouse cursor of Qt tells where the text fields are: no click, so nothing changes.
        const x = Math.round(viewport.width * 0.6);
        const runs = [];
        for (let y = 40; y < viewport.height * 0.6; y += 3) {
            await page.mouse.move(x, y);
            await page.waitForTimeout(20);
            const cursor = await page.evaluate(() => window.hpq_cursor());
            if (runs.length === 0 || runs[runs.length - 1].cursor !== cursor)
                runs.push({ cursor, from: y, to: y });
            else
                runs[runs.length - 1].to = y;
        }
        console.log(`cursor runs at x=${x}: ${JSON.stringify(runs)}`);
        const text_run = runs.find(run => run.cursor === 'text');
        if (!text_run) {
            console.log('RESULT: no text field found with the mouse cursor.');
            return;
        }
        const y = Math.round((text_run.from + text_run.to) / 2);
        await page.mouse.move(5, 5);

        show_console = true;
        await page.evaluate(() => { window.hpq_verbose = true; });
        const step = async (title, action) => {
            console.log(`--- ${title}`);
            await action();
            await page.waitForTimeout(600);
            console.log(`  => ${await page.evaluate(() => window.hpq_state())}`);
        };
        await step(`first tap on the line edit (${x}, ${y})`, () => page.touchscreen.tap(x, y));
        await step('second tap on the line edit', () => page.touchscreen.tap(x, y));
        await step('keyboard.type("ab")', () => page.keyboard.type('ab', { delay: 50 }));
        await step('composition "cd" then commit', async () => {
            if (browser_name !== 'chromium')
                return page.keyboard.insertText('cd');
            const session = await context.newCDPSession(page);
            await session.send('Input.imeSetComposition', { text: 'c', selectionStart: 1, selectionEnd: 1 });
            await page.waitForTimeout(100);
            await session.send('Input.imeSetComposition', { text: 'cd', selectionStart: 2, selectionEnd: 2 });
            await page.waitForTimeout(100);
            await session.send('Input.insertText', { text: 'cd' });
        });
        await step('tap again: Qt copies the text of the widget in its input element', () => page.touchscreen.tap(x, y));
    } finally {
        await browser.close();
    }
}

async function main()
{
    const site_directory = path.resolve(process.argv[2] ?? '');
    const browser_name = process.argv[3] ?? 'chromium';
    const device_name = process.argv[4] ?? 'Pixel 7';
    const server = await serve(site_directory);
    try {
        const site_url = `http://127.0.0.1:${server.address().port}/`;
        await run(site_url, browser_name, device_name, false);
        await run(site_url, browser_name, device_name, true);
    } finally {
        server.close();
    }
}

if (require.main === module) {
    main().then(() => process.exit(0), error => {
        console.error(error);
        process.exit(1);
    });
}
