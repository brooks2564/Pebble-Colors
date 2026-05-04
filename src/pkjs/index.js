var settings = {};
try { settings = JSON.parse(localStorage.getItem('colors_settings') || '{}'); } catch(e) {}

function buildPage(s) {
  var colors = [
    { label: 'Green',  hex: '#55FF00' },
    { label: 'Cyan',   hex: '#00FFFF' },
    { label: 'Yellow', hex: '#FFFF00' },
    { label: 'Orange', hex: '#FFAA00' },
    { label: 'Red',    hex: '#FF5500' },
    { label: 'Blue',   hex: '#5555FF' },
    { label: 'White',  hex: '#FFFFFF' },
    { label: 'Pink',   hex: '#FF55FF' }
  ];
  var sel = (s.color !== undefined) ? parseInt(s.color) : 0;
  var swatches = colors.map(function(c, i) {
    var active = (i === sel) ? 'border:3px solid #fff;transform:scale(1.2);' : 'border:3px solid transparent;';
    return '<div class="swatch" data-i="' + i + '" style="background:' + c.hex + ';' + active + '" title="' + c.label + '"></div>';
  }).join('');

  function tog(id, label, checked) {
    var chk = checked ? ' checked' : '';
    return '<div class="row"><span>' + label + '</span>' +
      '<label class="sw"><input type="checkbox" id="' + id + '"' + chk + '><span class="sl"></span></label></div>';
  }

  var html = '<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">' +
    '<style>*{box-sizing:border-box}body{font-family:-apple-system,sans-serif;background:#1a1a2e;color:#eee;margin:0;padding:16px}' +
    'h1{text-align:center;font-size:18px;margin-bottom:16px}' +
    '.card{background:#16213e;border-radius:10px;padding:14px;margin-bottom:14px}' +
    '.card-title{font-size:11px;color:#888;text-transform:uppercase;letter-spacing:1px;margin-bottom:10px}' +
    '.swatches{display:flex;flex-wrap:wrap;gap:10px;justify-content:center}' +
    '.swatch{width:40px;height:40px;border-radius:50%;cursor:pointer;transition:all .15s}' +
    '.row{display:flex;justify-content:space-between;align-items:center;padding:9px 0;border-bottom:1px solid #1a1a2e}' +
    '.row:last-child{border:none}.row span{font-size:14px}' +
    '.sw{position:relative;width:44px;height:26px;flex-shrink:0}' +
    '.sw input{opacity:0;width:0;height:0}' +
    '.sl{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#444;border-radius:26px;transition:.2s}' +
    '.sl:before{position:absolute;content:"";height:20px;width:20px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.2s}' +
    'input:checked+.sl{background:#55FF00}input:checked+.sl:before{transform:translateX(18px)}' +
    '.save{display:block;width:100%;padding:13px;background:#55FF00;color:#000;border:none;border-radius:10px;font-size:16px;font-weight:bold;cursor:pointer;margin-top:6px}' +
    '</style></head><body>' +
    '<h1>🎨 Colors Settings</h1>' +
    '<div class="card"><div class="card-title">Accent Color</div><div class="swatches">' + swatches + '</div></div>' +
    '<div class="card"><div class="card-title">Auto Color</div>' +
    tog('autoColor', 'Change Color Every Hour', !!s.autoColor) +
    tog('rainbow', 'Rainbow Mode (every minute)', !!s.rainbow) +
    '</div>' +
    '<div class="card"><div class="card-title">Display</div>' +
    tog('battery', 'Battery Bar', s.battery !== false) +
    tog('date', 'Date', s.date !== false) +
    tog('day', 'Day of Week', s.day !== false) +
    tog('steps', 'Step Counter', s.steps !== false) +
    tog('h24', '24-Hour Format', !!s.h24) +
    '</div>' +
    '<button class="save" id="save">Save Settings</button>' +
    '<script>var sel=' + sel + ';' +
    'document.querySelectorAll(".swatch").forEach(function(el){' +
    'el.addEventListener("click",function(){' +
    'document.querySelectorAll(".swatch").forEach(function(x){x.style.border="3px solid transparent";x.style.transform="";});' +
    'el.style.border="3px solid #fff";el.style.transform="scale(1.2)";sel=parseInt(el.dataset.i);});});' +
    'document.getElementById("save").addEventListener("click",function(){' +
    'var r={color:sel,battery:document.getElementById("battery").checked,steps:document.getElementById("steps").checked,' +
    'date:document.getElementById("date").checked,day:document.getElementById("day").checked,' +
    'h24:document.getElementById("h24").checked,autoColor:document.getElementById("autoColor").checked,' +
    'rainbow:document.getElementById("rainbow").checked};' +
    'document.location="pebblejs://close#"+encodeURIComponent(JSON.stringify(r));});' +
    '<\/script></body></html>';

  return 'data:text/html;base64,' + btoa(unescape(encodeURIComponent(html)));
}

Pebble.addEventListener('ready', function() {});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(buildPage(settings));
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response || e.response === 'CANCELLED') return;
  try {
    settings = JSON.parse(decodeURIComponent(e.response));
    localStorage.setItem('colors_settings', JSON.stringify(settings));
    var dict = {};
    if (settings.color     !== undefined) dict[0] = parseInt(settings.color);
    if (settings.battery   !== undefined) dict[1] = settings.battery   ? 1 : 0;
    if (settings.steps     !== undefined) dict[2] = settings.steps     ? 1 : 0;
    if (settings.date      !== undefined) dict[3] = settings.date      ? 1 : 0;
    if (settings.day       !== undefined) dict[4] = settings.day       ? 1 : 0;
    if (settings.h24       !== undefined) dict[5] = settings.h24       ? 1 : 0;
    if (settings.autoColor !== undefined) dict[6] = settings.autoColor ? 1 : 0;
    if (settings.rainbow   !== undefined) dict[7] = settings.rainbow   ? 1 : 0;
    Pebble.sendAppMessage(dict, function() {}, function() {});
  } catch(err) {}
});
