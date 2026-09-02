/* eslint-disable */
import puppeteer from "puppeteer";

(async () => {
  console.log("Starting puppeteer...");
  const browser = await puppeteer.launch({ headless: "new" });
  const page = await browser.newPage();

  page.on("console", (msg) => console.log("PAGE LOG:", msg.text()));
  page.on("pageerror", (err) => console.log("PAGE ERROR:", err.toString()));
  page.on("requestfailed", (request) =>
    console.log("REQUEST FAILED:", request.url(), request.failure().errorText),
  );

  console.log("Navigating to http://localhost:5173 ...");
  try {
    await page.goto("http://localhost:5173", {
      waitUntil: "networkidle0",
      timeout: 15000,
    });
  } catch (e) {
    console.log("Navigation error:", e.message);
  }

  const content = await page.content();
  console.log("HTML length:", content.length);
  if (content.includes('id="root"></div>')) {
    console.log("WARNING: Root might be empty!");
  }

  await browser.close();
  console.log("Done.");
})();
