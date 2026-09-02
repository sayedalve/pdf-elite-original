/* eslint-disable */
import puppeteer from "puppeteer";

(async () => {
  const browser = await puppeteer.launch();
  const page = await browser.newPage();
  page.on("console", (msg) => console.log("PAGE LOG:", msg.text()));
  page.on("pageerror", (err) => console.log("PAGE ERROR:", err.toString()));

  await page
    .goto("http://localhost:5173", {
      waitUntil: "networkidle0",
      timeout: 15000,
    })
    .catch((e) => console.log(e.message));

  const html = await page.evaluate(() => document.body.innerHTML);
  console.log("HTML:", html.substring(0, 1000));
  await browser.close();
})();
