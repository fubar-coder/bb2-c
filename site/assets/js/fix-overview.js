// Sorting and filtering of the table of the page "Important Fixes &
// Breaking Changes" (shortcode fix-overview). Without JavaScript the table
// stays in its initial order and complete; the buttons and the filters are
// made here, so that no dead controls exist without it.
(function () {
  "use strict";

  function setup(box) {
    var table = box.querySelector("table");
    var tbody = table.tBodies[0];
    var rows = Array.prototype.slice.call(tbody.rows);
    var initial = rows.slice();
    var total = rows.length;

    // the filters: severity and topic, with a live count
    var bar = document.createElement("div");
    bar.className = "fix-overview-filters";
    var count = document.createElement("p");
    count.className = "fix-overview-count";
    count.setAttribute("aria-live", "polite");

    function select(id, label, key, values, names) {
      var wrap = document.createElement("div");
      var l = document.createElement("label");
      l.htmlFor = id;
      l.textContent = label;
      var s = document.createElement("select");
      s.id = id;
      var all = document.createElement("option");
      all.value = "";
      all.textContent = "All";
      s.appendChild(all);
      values.forEach(function (v) {
        var o = document.createElement("option");
        o.value = v;
        o.textContent = names && names[v] ? v + ", " + names[v] : v;
        s.appendChild(o);
      });
      s.addEventListener("change", filter);
      wrap.appendChild(l);
      wrap.appendChild(s);
      bar.appendChild(wrap);
      return s;
    }

    function distinct(key) {
      var seen = {};
      var out = [];
      initial.forEach(function (r) {
        var v = r.dataset[key];
        if (!seen[v]) {
          seen[v] = true;
          out.push(v);
        }
      });
      return out;
    }

    var levelNames = {};
    initial.forEach(function (r) {
      var badge = r.querySelector(".fix-overview-level");
      if (badge) levelNames[r.dataset.severity] = badge.title.toLowerCase();
    });
    var severities = distinct("severity").sort();
    var topics = distinct("topic").sort(function (a, b) {
      return a.localeCompare(b);
    });
    var sevSelect = select("fix-overview-severity", "Level", "severity", severities, levelNames);
    var topicSelect = select("fix-overview-topic", "Topic", "topic", topics, null);

    // a filter given in the URL: ?level=3,4 (one level or several) and
    // ?topic=Name; several levels become an option of their own
    var params = new URLSearchParams(window.location.search);
    var wanted = (params.get("level") || "").split(",").filter(function (v) {
      return severities.indexOf(v) >= 0;
    });
    if (wanted.length > 1) {
      var several = document.createElement("option");
      several.value = wanted.join(",");
      several.textContent = wanted.slice(0, -1).join(", ") + " and " + wanted[wanted.length - 1];
      sevSelect.appendChild(several);
    }
    if (wanted.length) sevSelect.value = wanted.join(",");
    if (topics.indexOf(params.get("topic")) >= 0) topicSelect.value = params.get("topic");

    function filter() {
      var sev = sevSelect.value ? sevSelect.value.split(",") : [];
      var topic = topicSelect.value;
      var shown = 0;
      rows.forEach(function (r) {
        var ok = (!sev.length || sev.indexOf(r.dataset.severity) >= 0) && (!topic || r.dataset.topic === topic);
        r.hidden = !ok;
        if (ok) shown++;
      });
      count.textContent = shown === total
        ? "All " + total + " fixes are shown."
        : shown + " of " + total + " fixes are shown.";
    }

    // sorting: a button in every column header
    var headers = Array.prototype.slice.call(table.tHead.rows[0].cells);
    function keyOf(row, th) {
      var key = th.dataset.sort;
      return row.dataset[key];
    }
    function sortBy(th, dir) {
      var numeric = th.dataset.type === "number";
      var factor = dir === "ascending" ? 1 : -1;
      // a stable sort on the order before (Array.prototype.sort is stable)
      rows.sort(function (a, b) {
        var x = keyOf(a, th);
        var y = keyOf(b, th);
        var c = numeric ? Number(x) - Number(y) : x.localeCompare(y);
        return c * factor;
      });
      rows.forEach(function (r) {
        tbody.appendChild(r);
      });
      headers.forEach(function (h) {
        if (h === th) h.setAttribute("aria-sort", dir);
        else h.removeAttribute("aria-sort");
      });
    }
    headers.forEach(function (th) {
      var b = document.createElement("button");
      b.type = "button";
      while (th.firstChild) b.appendChild(th.firstChild);
      th.appendChild(b);
      b.addEventListener("click", function () {
        var dir = th.getAttribute("aria-sort") === "ascending" ? "descending" : "ascending";
        sortBy(th, dir);
      });
    });
    // the initial order is by severity, ascending
    headers.forEach(function (h) {
      if (h.dataset.sort === "severity") h.setAttribute("aria-sort", "ascending");
    });

    box.insertBefore(bar, table);
    box.insertBefore(count, table);
    filter();
  }

  function init() {
    Array.prototype.forEach.call(document.querySelectorAll(".fix-overview"), setup);
  }
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init);
  else init();
})();
