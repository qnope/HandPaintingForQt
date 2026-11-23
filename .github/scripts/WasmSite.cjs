// Shared by the tests of the WebAssembly site: serves the directory of the site over HTTP and
// starts the browser. Needs the "playwright" package. The browser is CHROME_BIN when set
// (GitHub runners), otherwise the Chromium of Playwright.
const fs = require('fs');
const http = require('http');
const path = require('path');
const { chromium } = require('playwright');

const StartTimeoutMs = 120000;
const ContentTypes = {
    '.html': 'text/html; charset=utf-8',
    '.js': 'text/javascript',
    '.wasm': 'application/wasm',
    '.svg': 'image/svg+xml',
    '.ttf': 'font/ttf',
    '.txt': 'text/plain; charset=utf-8',
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
        const content_type = ContentTypes[path.extname(file_path)] ?? 'application/octet-stream';
        response.writeHead(200, { 'Content-Type': content_type });
        fs.createReadStream(file_path).pipe(response);
    });
    return new Promise(resolve => server.listen(0, '127.0.0.1', () => resolve(server)));
}

function site_url(server)
{
    return `http://127.0.0.1:${server.address().port}/`;
}

function launch_browser()
{
    return chromium.launch(process.env.CHROME_BIN ? { executablePath: process.env.CHROME_BIN } : {});
}

// The directory of the site, from the first argument of the script; undefined when it holds no site.
function site_directory_argument()
{
    const site_directory = path.resolve(process.argv[2] ?? '');
    const has_site = process.argv.length >= 3 && fs.existsSync(path.join(site_directory, 'index.html'));
    return has_site ? site_directory : undefined;
}

module.exports = { StartTimeoutMs, serve, site_url, launch_browser, site_directory_argument };
