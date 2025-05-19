const express = require("express");
const dgram = require("dgram");
const path = require("path");

const app = express();
const PORT = 8080;
const enablePrint = process.argv.includes("-p");

const udp = dgram.createSocket("udp4");

// Init fallback values
let lastKnownCoords = {};
for (let i = 0; i < 11; i++) {
  lastKnownCoords[`id${i}`] = [0, 0];
}

// Handle incoming UDP messages
udp.on("message", (msg) => {
  const text = msg.toString().trim().replace(/[;,]/g, "");
  const values = text.split(/\s+/).map(Number);

  if (values.length === 23 && values.every(n => !isNaN(n))) {
    const newCoords = {};

    // Positions: 11 Körper à 2 Werte = 22 Werte
    for (let i = 0; i < 11; i++) {
      const x = values[i * 2];
      const y = values[i * 2 + 1];
      newCoords[`id${i}`] = [x, y];
    }

    // Radiuswert (Wert 23 = Index 22)
    const radiusValue = values[22];
    newCoords["radius"] = radiusValue;

    lastKnownCoords = newCoords;

    if (enablePrint) {
      const line = Object.entries(newCoords)
        .filter(([key]) => key.startsWith("id")) // nur Körper id0–id10 loggen
        .map(([id, [x, y]]) => `${id}:x=${x.toFixed(2)} y=${y.toFixed(2)}`)
        .join(" | ");
      console.log(line + ` | radius=${radiusValue.toFixed(2)}`);
    }
  } else {
    console.warn("Invalid UDP data received:", text);
    console.warn("Invalid data:", values, "length:", values.length);
  }
});

// Start UDP server
udp.bind(5000, () => {
  console.log("UDP listener active on port 5000");
});

// Serve frontend
app.use(express.static(path.join(__dirname)));

// REST-API: get latest coords
app.get("/data", (req, res) => {
  res.json(lastKnownCoords);
});

// Start HTTP server
app.listen(PORT, () => {
  console.log(`Start frontend: http://localhost:${PORT}`);
});
