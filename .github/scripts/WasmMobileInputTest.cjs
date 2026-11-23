// Test of the WebAssembly site on a phone: opens it in a browser that emulates a phone, taps the
// line edit and types, and checks that the page keeps the focus where Qt puts it to open the
// virtual keyboard, and that the text reaches the widget.
//
// An emulated phone has no virtual keyboard and is not Safari on iOS: the test checks what the
// keyboard depends on, not the keyboard itself.
//
// Usage: node WasmMobileInputTest.cjs <site directory>
// Needs the "playwright" package (see WasmSite.cjs for the browser).
const { devices } = require('playwright');
const { StartTimeoutMs, serve, site_url, launch_browser, site_directory_argument } = require('./WasmSite.cjs');

const Phone = 'Pixel 7';
// Time left to the application and to the browser after an action, before looking at the page.
const SettleMs = 300;
const TextTyped = 'ab';
const TextComposed = 'cd';
// Events the browser makes of a tap, after the pointer and touch events.
const MouseEventsOfTap = ['mousedown', 'mouseup', 'click'];

// Runs in the page, before its scripts: what the test reads from the page.
function install_probes()
{
    const find_in_shadow_trees = (root, predicate) => {
        for (const element of root.querySelectorAll('*')) {
            if (predicate(element))
                return element;
            const found = element.shadowRoot ? find_in_shadow_trees(element.shadowRoot, predicate) : null;
            if (found)
                return found;
        }
        return null;
    };
    // Qt draws each window in a shadow tree of the container element.
    const window_contents = () => find_in_shadow_trees(
        document, element => element.classList.contains('qt-window-contents'));

    window.hpq_probes = {
        // The hidden text field Qt gives the focus to, for the virtual keyboard to open.
        text_field: () => document.querySelector('body > input'),
        text_field_has_focus: () => document.activeElement === window.hpq_probes.text_field(),
        text_field_value: () => window.hpq_probes.text_field()?.value,
        text_field_attributes: () => {
            const text_field = window.hpq_probes.text_field();
            return ['autocorrect', 'autocomplete', 'spellcheck'].map(name => `${name}=${text_field?.getAttribute(name)}`).join(' ');
        },
        // CSS name of the mouse cursor of the widget under the mouse.
        cursor: () => window_contents()?.style.cursor,
        // Mouse events seen since the last call.
        mouse_events: [],
        take_mouse_events: () => window.hpq_probes.mouse_events.splice(0),
        // What Safari on iOS does after a tap, even when the page cancelled the pointer events:
        // a mousedown, whose default action moves the focus. True when the page cancels it.
        mouse_down_of_tap_is_cancelled: () => {
            const canvas = find_in_shadow_trees(document, element => element.tagName === 'CANVAS');
            return !canvas.dispatchEvent(new MouseEvent('mousedown', { bubbles: true, cancelable: true, composed: true }));
        },
    };
    for (const type of ['mousedown', 'mouseup', 'click'])
        window.addEventListener(type, () => window.hpq_probes.mouse_events.push(type), true);
}

// The widgets with the cursor of a text at the horizontal position x, from the top: for each, its
// first and last vertical positions. Moving the mouse changes nothing in the application, and Qt
// gives the page the cursor of the widget under the mouse. Down the form, the first is the line
// edit and the second the editable font combo box.
async function find_text_widgets(page, x, height)
{
    const StepPx = 3;
    const widgets = [];
    for (let y = 0; y < height; y += StepPx) {
        await page.mouse.move(x, y);
        await page.waitForTimeout(20);
        if (await page.evaluate(() => window.hpq_probes.cursor()) !== 'text')
            continue;
        const last_widget = widgets[widgets.length - 1];
        if (last_widget && last_widget.last === y - StepPx)
            last_widget.last = y;
        else
            widgets.push({ first: y, last: y });
    }
    // The mouse leaves the widgets: the taps come next.
    await page.mouse.move(x, 0);
    return widgets;
}

// True when a text editor is under the point: the mouse gets the cursor of a text there.
async function has_text_editor_at(page, x, y)
{
    await page.mouse.move(x, y);
    await page.waitForTimeout(50);
    const cursor = await page.evaluate(() => window.hpq_probes.cursor());
    await page.mouse.move(x, 0);
    return cursor === 'text';
}

// Double taps down the table, below the form, until a cell opens its editor. Returns the vertical
// position of the editor, undefined when no cell opened one.
async function open_cell_editor(page, x, from_y, to_y)
{
    const StepPx = 16;
    // Longer than the interval of a double click: the taps of two attempts must not add up.
    const BetweenAttemptsMs = 700;
    for (let y = from_y; y < to_y; y += StepPx) {
        await page.touchscreen.tap(x, y);
        await page.touchscreen.tap(x, y);
        await page.waitForTimeout(SettleMs);
        if (await has_text_editor_at(page, x, y))
            return y;
        await page.waitForTimeout(BetweenAttemptsMs);
    }
    return undefined;
}

async function main()
{
    const site_directory = site_directory_argument();
    if (!site_directory) {
        console.error('Usage: node WasmMobileInputTest.cjs <site directory>');
        return 2;
    }

    const server = await serve(site_directory);
    const browser = await launch_browser();
    const errors = [];
    const expect = (condition, message) => {
        console.log(`${condition ? 'ok  ' : 'FAIL'} ${message}`);
        if (!condition)
            errors.push(message);
    };
    try {
        const context = await browser.newContext({ ...devices[Phone] });
        const page = await context.newPage();
        page.on('pageerror', error => errors.push(`Uncaught error in the page: ${error.message}`));
        await page.addInitScript(install_probes);
        await page.goto(site_url(server));
        await page.waitForFunction(() => document.documentElement.dataset.state === 'running',
                                   undefined, { timeout: StartTimeoutMs });
        await page.waitForFunction(() => window.hpq_probes.cursor() !== undefined, undefined, { timeout: StartTimeoutMs });
        await page.waitForTimeout(2000);
        const probe = name => page.evaluate(probe_name => window.hpq_probes[probe_name](), name);

        const viewport = page.viewportSize();
        const x = Math.round(viewport.width * 0.6);
        const text_widgets = await find_text_widgets(page, x, Math.round(viewport.height * 0.6));
        if (text_widgets.length < 2) {
            errors.push(`The line edit and the font combo box were not found: ${JSON.stringify(text_widgets)}.`);
            return 1;
        }
        const [line_edit, font_combo_box] = text_widgets;
        const y = Math.round((line_edit.first + line_edit.last) / 2);
        console.log(`${Phone}, ${viewport.width} x ${viewport.height}: line edit at (${x}, ${y})`);
        const tap = async () => {
            await probe('take_mouse_events');
            await page.touchscreen.tap(x, y);
            await page.waitForTimeout(SettleMs);
        };

        await tap();
        expect(await probe('text_field_has_focus'), 'A tap on the line edit gives the focus to the text field of Qt.');
        const mouse_events = await probe('take_mouse_events');
        expect(mouse_events.length === 0,
               `The browser makes no mouse event of the tap (${MouseEventsOfTap.join(', ')}): got [${mouse_events.join(', ')}].`);
        expect(await probe('mouse_down_of_tap_is_cancelled'), 'A mousedown that follows the tap is cancelled.');
        expect(await probe('text_field_has_focus'), 'The text field of Qt still has the focus after this mousedown.');
        const attributes = await probe('text_field_attributes');
        expect(attributes === 'autocorrect=off autocomplete=off spellcheck=false',
               `The corrections of the keyboard are switched off in the text field of Qt: ${attributes}.`);

        await page.keyboard.type(TextTyped, { delay: 50 });
        // A phone keyboard composes the word being typed, then commits it.
        const session = await context.newCDPSession(page);
        for (let length = 1; length <= TextComposed.length; ++length) {
            await session.send('Input.imeSetComposition',
                               { text: TextComposed.slice(0, length), selectionStart: length, selectionEnd: length });
            await page.waitForTimeout(100);
        }
        await session.send('Input.insertText', { text: TextComposed });
        await page.waitForTimeout(SettleMs);

        // On a tap, Qt copies the text of the widget to its text field.
        await tap();
        const text = await probe('text_field_value');
        expect(text === TextTyped + TextComposed,
               `The text typed and composed reaches the line edit: "${text}", expected "${TextTyped + TextComposed}".`);
        expect(await probe('text_field_has_focus'), 'The text field of Qt has the focus after a tap on the focused line edit.');

        // A cell of the table, on the left below the form: a double tap opens its editor. The
        // editor gets the focus without being tapped itself.
        const cell_x = Math.round(viewport.width * 0.25);
        const cell_y = await open_cell_editor(page, cell_x, font_combo_box.last + 40, font_combo_box.last + 300);
        console.log(`Editor of a cell at (${cell_x}, ${cell_y})`);
        if (cell_y === undefined) {
            errors.push('No cell of the table opened its editor on a double tap.');
            return 1;
        }
        expect(await probe('text_field_has_focus'),
               'A double tap on a cell gives the focus to the text field of Qt, with the editor of the cell.');
        const cell_text = await probe('text_field_value');
        expect(typeof cell_text === 'string' && cell_text.length > 0,
               `The text field of Qt holds the text of the cell: "${cell_text}".`);
        // DIAGNOSTIC (temporary): what one more tap in the open editor does.
        await page.touchscreen.tap(cell_x, cell_y);
        await page.waitForTimeout(SettleMs);
        console.log(`After one more tap in the editor: focus=${await probe('text_field_has_focus')} value="${await probe('text_field_value')}"`);
    } finally {
        await browser.close();
        server.close();
    }

    for (const error of errors)
        console.error(`ERROR: ${error}`);
    console.log(errors.length === 0 ? 'Text can be typed on a phone.' : 'Mobile input test failed.');
    return errors.length === 0 ? 0 : 1;
}

main().then(code => process.exit(code), error => {
    console.error(error);
    process.exit(1);
});
