window.onerror = function (msg, url, line, col, error) {
  var d = document.createElement("div");
  d.style =
    "color:red;padding:20px;z-index:9999;position:absolute;background:white;top:0;left:0;width:100%;height:100%;";
  d.innerHTML =
    "<h1>Error Catcher</h1><pre>" +
    msg +
    "<br>" +
    (error && error.stack ? error.stack : "") +
    "</pre>";
  document.body.appendChild(d);
};
window.addEventListener("unhandledrejection", function (e) {
  var d = document.createElement("div");
  d.style =
    "color:red;padding:20px;z-index:9999;position:absolute;background:white;top:0;left:0;width:100%;height:100%;";
  d.innerHTML =
    "<h1>Unhandled Promise Rejection</h1><pre>" +
    e.reason +
    "<br>" +
    (e.reason && e.reason.stack ? e.reason.stack : "") +
    "</pre>";
  document.body.appendChild(d);
});
