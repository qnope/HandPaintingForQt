// Smoke test of the WebAssembly site: serves the directory over HTTP, opens it in a browser
// and checks that the application starts and draws a window.
//
// Usage: node WasmSmokeTest.cjs <site directory> [screenshot.png]
// Needs the "playwright" package (see WasmSite.cjs for the browser).
const { StartTimeoutMs, serve, site_url, launch_browser, site_directory_argument } = require('./WasmSite.cjs');

const WindowTimeoutMs = 30000;

// Sizes of the canvases of the page, shadow trees included: Qt draws each window in a canvas
// placed in a shadow tree of the container element.
function canvas_sizes()
{
    const sizes = [];
    const visit = root => {
        for (const element of root.querySelectorAll('*')) {
            if (element.tagName === 'CANVAS')
                sizes.push({ width: element.width, height: element.height });
            if (element.shadowRoot)
                visit(element.shadowRoot);
        }
    };
    visit(document);
    return sizes;
}

async function main()
{
    const site_directory = site_directory_argument();
    const screenshot_path = process.argv[3];
    if (!site_directory) {
        console.error('Usage: node WasmSmokeTest.cjs <site directory> [screenshot.png]');
        return 2;
    }

    const server = await serve(site_directory);
    const browser = await launch_browser();
    const errors = [];
    try {
        const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
        page.on('console', message => console.log(`[browser ${message.type()}] ${message.text()}`));
        page.on('pageerror', error => errors.push(`Uncaught error in the page: ${error.message}`));
        page.on('requestfailed', request => errors.push(`Request failed: ${request.url()}`));
        page.on('response', response => {
            if (response.status() >= 400)
                errors.push(`HTTP ${response.status()}: ${response.url()}`);
        });

        await page.goto(site_url(server));
        const state = () => page.evaluate(() => document.documentElement.dataset.state);
        try {
            await page.waitForFunction(() => document.documentElement.dataset.state !== 'loading',
                                       undefined, { timeout: StartTimeoutMs });
        } catch (error) {
            errors.push(`The application did not start within ${StartTimeoutMs / 1000} s.`);
        }
        if (await state() === 'running') {
            try {
                await page.waitForFunction(
                    canvas_sizes_source => new Function(`return (${canvas_sizes_source})()`)()
                        .some(size => size.width > 0 && size.height > 0),
                    canvas_sizes.toString(), { timeout: WindowTimeoutMs });
            } catch (error) {
                errors.push(`The application drew no window within ${WindowTimeoutMs / 1000} s.`);
            }
            // Lets the first frames reach the screen before the screenshot.
            await page.waitForTimeout(2000);
        }

        const final_state = await state();
        console.log(`State of the page: ${final_state}`);
        console.log(`Status text: ${await page.textContent('#status-text')}`);
        console.log(`Canvases: ${JSON.stringify(await page.evaluate(canvas_sizes))}`);
        if (final_state !== 'running')
            errors.push(`The page is in state "${final_state}" instead of "running".`);
        if (screenshot_path) {
            await page.screenshot({ path: screenshot_path });
            console.log(`Screenshot: ${screenshot_path}`);
        }
    } finally {
        await browser.close();
        server.close();
    }

    for (const error of errors)
        console.error(`ERROR: ${error}`);
    console.log(errors.length === 0 ? 'The application runs in the browser.' : 'Smoke test failed.');
    return errors.length === 0 ? 0 : 1;
}

main().then(code => process.exit(code), error => {
    console.error(error);
    process.exit(1);
});
