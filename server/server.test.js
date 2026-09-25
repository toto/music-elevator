const assert = require("node:assert/strict");
const { after, before, test } = require("node:test");
const app = require("./server");

let server;
let baseUrl;

before(async () => {
  server = app.listen(0, "127.0.0.1");
  await new Promise((resolve) => server.once("listening", resolve));
  baseUrl = `http://127.0.0.1:${server.address().port}`;
});

after(() => server.close());

test("stores a valid sensor reading and rejects invalid input", async () => {
  const accepted = await fetch(`${baseUrl}/data`, {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({ pressure: 1016.82, height: 1.43 }),
  });
  assert.equal(accepted.status, 200);

  const reading = await (await fetch(`${baseUrl}/data`)).json();
  assert.equal(reading.pressure, 1016.82);
  assert.equal(reading.height, 1.43);

  const rejected = await fetch(`${baseUrl}/data`, {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({ pressure: "bad", height: 1.43 }),
  });
  assert.equal(rejected.status, 400);
});
