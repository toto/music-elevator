const express = require("express");

const app = express();
const port = process.env.PORT || 5000;

app.use(express.json({ limit: "1kb" }));

let latestReading = null;

app.post("/data", (req, res) => {
  const { pressure, height } = req.body ?? {};

  if (!Number.isFinite(pressure) || !Number.isFinite(height)) {
    return res.status(400).json({ ok: false, error: "pressure and height must be numbers" });
  }

  latestReading = { pressure, height, receivedAt: new Date().toISOString() };
  console.log("Received:", latestReading);
  res.json({ ok: true });
});

app.get("/data", (_req, res) => {
  res.json(latestReading || { message: "No readings received yet" });
});

app.get("/", (_req, res) => {
  res.send(`<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="refresh" content="1">
    <title>ESP32 Elevator Sensor</title>
  </head>
  <body>
    <h1>ESP32 Elevator Sensor</h1>
    ${latestReading
      ? `<p><strong>Pressure:</strong> ${latestReading.pressure} hPa</p>
         <p><strong>Height:</strong> ${latestReading.height} m</p>
         <p><strong>Received:</strong> ${latestReading.receivedAt}</p>`
      : "<p>Waiting for ESP32...</p>"}
  </body>
</html>`);
});

if (require.main === module) {
  app.listen(port, "0.0.0.0", () => {
    console.log(`Server running at http://localhost:${port}`);
  });
}

module.exports = app;

