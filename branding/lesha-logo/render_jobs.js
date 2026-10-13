const { chromium } = require('playwright');
const jobs = require(process.argv[2]);
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  for (const j of jobs) {
    await p.setViewportSize({ width: j.w, height: j.h });
    await p.goto('file://' + j.svg);
    await p.screenshot({ path: j.png, omitBackground: true, clip: { x: 0, y: 0, width: j.w, height: j.h } });
  }
  await b.close();
})();
