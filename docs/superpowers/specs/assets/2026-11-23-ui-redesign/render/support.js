// Stand-in for the design canvas runtime: renders a .dc.html board statically (first frame,
// no events) so it can be screenshotted. Query: ?openPage=<page>&beat=<n>&message=<text>&editTempo=<n>
// <dc-import name="X" prop-a="..."> loads X.dc.html (sync XHR; needs --allow-file-access-from-files)
// and renders it with its own styles inside a shadow root.
class DCLogic {
  constructor (props) { this.props = props || {}; this.state = null; }
  setState (update) { this.state = Object.assign ({}, this.state, update); }
}
window.DCLogic = DCLogic;

function evaluate (expression, scope) {
  try { return new Function ('scope', 'with (scope) { return (' + expression + '); }') (scope); }
  catch (error) { console.warn ('eval failed', expression, error); return ''; }
}
function interpolate (text, scope) {
  return text.replace (/\{\{([^}]+)\}\}/g, (match, expression) => {
    const value = evaluate (expression.trim(), scope);
    return value === undefined || value === null ? '' : String (value);
  });
}
function soleExpression (text) { const found = /^\{\{([^}]+)\}\}$/.exec ((text || '').trim()); return found ? found[1].trim() : null; }
function camel (name) { return name.replace (/-([a-z])/g, (match, letter) => letter.toUpperCase()); }

function loadBoard (name) {
  const request = new XMLHttpRequest();
  request.open ('GET', name + '.dc.html', false);
  request.send();
  return new DOMParser().parseFromString (request.responseText, 'text/html');
}

// Renders a parsed board: returns { styles: [nodes], content: DocumentFragment }.
function renderBoard (boardDocument, props, query) {
  const host = boardDocument.querySelector ('x-dc');
  const helmet = host.querySelector ('helmet');
  const styles = helmet ? Array.from (helmet.childNodes).filter ((node) => node.nodeType === Node.ELEMENT_NODE && node.tagName.toLowerCase() === 'style') : [];
  const script = boardDocument.querySelector ('script[type="text/x-dc"]');
  const content = document.createDocumentFragment();
  if (! script) return { styles, content };
  const Component = new Function ('DCLogic', script.textContent + '\nreturn Component;') (DCLogic);
  const component = new Component (props);
  if (query) {
    if (query.get ('beat')) component.setState ({ beatCount: parseInt (query.get ('beat'), 10) });
    if (query.get ('message')) component.setState ({ message: query.get ('message') });
    if (query.get ('editTempo')) component.setState ({ tempoEditing: true, tempoDraft: query.get ('editTempo') });
  }
  const values = component.renderVals();
  Array.from (host.childNodes).forEach ((child) => { if (child !== helmet && child !== script) render (child, values, content); });
  return { styles, content };
}

function render (node, scope, parent) {
  if (node.nodeType === Node.TEXT_NODE) { parent.appendChild (document.createTextNode (interpolate (node.textContent, scope))); return; }
  if (node.nodeType !== Node.ELEMENT_NODE) return;
  const tag = node.tagName.toLowerCase();
  if (tag === 'sc-for') {
    const list = evaluate (soleExpression (node.getAttribute ('list')), scope) || [];
    const name = node.getAttribute ('as');
    list.forEach ((item, index) => {
      const inner = Object.create (scope); inner[name] = item; inner.$index = index;
      node.childNodes.forEach ((child) => render (child, inner, parent));
    });
    return;
  }
  if (tag === 'sc-if') {
    if (evaluate (soleExpression (node.getAttribute ('value')), scope)) node.childNodes.forEach ((child) => render (child, scope, parent));
    return;
  }
  if (tag === 'dc-import') {
    const props = {};
    let width = '100%', height = '100%';
    for (const attribute of node.attributes) {
      if (attribute.name === 'name' || /^on-/.test (attribute.name)) continue;
      if (attribute.name === 'hint-size') { [width, height] = attribute.value.split (','); continue; }
      const sole = soleExpression (attribute.value);
      props[camel (attribute.name)] = sole ? evaluate (sole, scope) : interpolate (attribute.value, scope);
    }
    const box = document.createElement ('div');
    box.style.cssText = 'position:relative;width:' + width.trim() + ';height:' + height.trim();
    const shadow = box.attachShadow ({ mode: 'open' });
    const imported = renderBoard (loadBoard (node.getAttribute ('name')), props, null);
    imported.styles.forEach ((style) => shadow.appendChild (style.cloneNode (true)));
    shadow.appendChild (imported.content);
    parent.appendChild (box);
    return;
  }
  if (tag === 'script') return;
  const copy = document.createElement (tag);
  for (const attribute of node.attributes) {
    if (/^on/i.test (attribute.name) || attribute.name === 'autofocus') continue;
    copy.setAttribute (attribute.name, interpolate (attribute.value, scope));
  }
  if (tag === 'input') copy.value = copy.getAttribute ('value') || '';
  node.childNodes.forEach ((child) => render (child, scope, copy));
  parent.appendChild (copy);
}

document.addEventListener ('DOMContentLoaded', () => {
  const local = document.createElement ('style');
  local.textContent = "@font-face{font-family:'Barlow Semi Condensed';font-weight:500;src:url(fonts/BarlowSemiCondensed-Medium.ttf)}"
                    + "@font-face{font-family:'Barlow Semi Condensed';font-weight:600;src:url(fonts/BarlowSemiCondensed-SemiBold.ttf)}"
                    + "@font-face{font-family:'Share Tech Mono';src:url(fonts/ShareTechMono-Regular.ttf)}";
  document.head.appendChild (local);
  document.querySelectorAll ('link[href*="fonts.googleapis"]').forEach ((link) => link.remove());

  const query = new URLSearchParams (location.search);
  const props = {}; if (query.get ('openPage')) props.openPage = query.get ('openPage');
  const board = renderBoard (document, props, query);
  board.styles.forEach ((style) => document.head.appendChild (style.cloneNode (true)));
  const host = document.querySelector ('x-dc');
  host.replaceWith (board.content);
  document.body.setAttribute ('data-rendered', 'yes');
});
