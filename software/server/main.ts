import pg from "npm:pg";
const { Pool } = pg;

interface SniffPayload {
  co2: number;
  humidity: number;
  temperature: number;
  data: string;
}
interface Sniff extends SniffPayload {
  sniff_id: number;
  created_at: string | Date;
}





//==============================
// PAYLOAD
//==============================
function isValidPayload(body: unknown): body is SniffPayload {
  if (typeof body !== "object" || body === null) {
    return false;
  }
  const obj = body as Record<string, unknown>;
  return (
    typeof obj?.co2 == "number" &&
    typeof obj.humidity == "number" &&
    typeof obj.temperature == "number" &&
    typeof obj?.data == "string"
  );
}
function isValidPayloadArray(body: unknown): body is SniffPayload[] {
  return Array.isArray(body) && body.length > 0 && body.every(isValidPayload);
}




//==============================
// DATABASE
//==============================
const databaseUrl = Deno.env.get("DATABASE_URL");
const pool = new Pool({
  connectionString: databaseUrl,
});


//==============================
// WEBSOCKET CONNECTIONS & BROADCAST
//==============================

const clients = new Set<WebSocket>();

function broadcastSniff(sniff: Sniff) {
    const payload = JSON.stringify({ type: "latest_sniff", data: sniff });
    for (const client of clients) {
        if (client.readyState == WebSocket.OPEN) {
            client.send(payload);
        }
    }
}


//==============================
// FUNCTIONS
//==============================

async function insertSniff({ co2, humidity, temperature, data }: SniffPayload): Promise<Sniff> {
  const query =
    `INSERT INTO sniffs (co2, humidity, temperature, data) VALUES ($1,$2,$3, $4) RETURNING sniff_id, co2, humidity, temperature, data, created_at;`;
  const result = await pool.query(query, [co2, humidity, temperature, data]);
  const newSniff = result.rows[0];
  broadcastSniff(newSniff);
  return result.rows[0];
}


async function insertManySniffs(sniffs: SniffPayload[]): Promise<Sniff[]> {
  if (sniffs.length === 0) return [];
  const valuePlaceholders: string[] = [];
  const params: unknown[] = [];
  sniffs.forEach((sniff, index) => {
    const offset = index * 4;
    valuePlaceholders.push(`($${offset +1}, $${offset + 2}, $${offset + 3}, $${offset + 4})`);
    params.push(sniff.co2, sniff.humidity, sniff.temperature, sniff.data);
  });
  const query = `INSERT INTO sniffs (co2, humidity, temperature, data) VALUES ${valuePlaceholders.join(", ")} RETURNING sniff_id, co2, humidity, temperature, data, created_at;`;
  const result = await pool.query(query, params);
  const inserted_sniffs = result.rows;
  if(inserted_sniffs.length > 0) {
    broadcastSniff(inserted_sniffs[inserted_sniffs.length-1]);
  }
  return inserted_sniffs;
}


async function selectAllSniffs(): Promise<Sniff[]> {
  const query =
    `SELECT sniff_id, co2, humidity, temperature, data, created_at FROM sniffs ORDER BY created_at DESC;`;
  const result = await pool.query(query);
  return result.rows;
}

async function selectLatestSniff():Promise<Sniff | null> {
    const query =
     `SELECT sniff_id, co2, humidity, temperature, data, created_at FROM sniffs ORDER BY created_at DESC LIMIT 1;`;
    const result = await pool.query(query);
    return result.rows[0] || null;
}

//==============================
// SERVER
//==============================
const port = Number(Deno.env.get("PORT")) || 3000;
const hostname = "0.0.0.0";

console.log("Processing server...");
Deno.serve({ hostname, port }, async (req: Request) => {
  console.log("Processing request...");
  const url = new URL(req.url);

  //==============================
  // WEBSOCKET
  //==============================

  console.log("Checking websocket");
  if (req.headers.get("upgrade") === "websocket") {
    const { socket, response } = Deno.upgradeWebSocket(req);

    socket.onopen = async () => {
      clients.add(socket);
      try {
        const latest = await selectLatestSniff();
        if ( latest && socket.readyState === WebSocket.OPEN) {
          socket.send(JSON.stringify({ type: "latest_sniff", data: latest }))
        }
      } catch (err) {
        console.error("Error sending initial latest sniff:", err);
      }
    }
    socket.onmessage = async (event) => {
      try {
        const payload = JSON.parse(event.data);

        if (typeof payload !== "object" || payload === null) {
          socket.send(JSON.stringify({ error: "Invalid message format" }));
          return;
        }
        const command = (payload as Record<string, unknown>).command;

        switch (command) {
          case "insert": {
            const sniff = (payload as Record<string, unknown>).sniff;
            const payload_id = (payload as Record<string, unknown>).payload_id;
            if (!isValidPayload(sniff)) {
              socket.send(JSON.stringify({ error: "Invalid payload format" }));
              return;
            }
            await insertSniff(sniff);
            socket.send(
              JSON.stringify({ status: "success", payload_id: payload_id }),
            );
            break;
          }

          case "insertMany": {
            const sniffs = (payload as Record<string, unknown>).sniffs ??
              (payload as Record<string, unknown>).sniff;
            const payload_id = (payload as Record<string, unknown>).payload_id;
            if (!isValidPayloadArray(sniffs)) {
              socket.send(
                JSON.stringify({ error: "Invalid sniffs array payload" }),
              );
              return;
            }
            await insertManySniffs(sniffs);
            socket.send(
              JSON.stringify({ status: "success", payload_id: payload_id }),
            );
            break;
          }

          case "selectAll": {
            const records = await selectAllSniffs();
            socket.send(
              JSON.stringify({ status: "success", records: records }),
            );
            break;
          }

          case "getLatest": {
            const latest = await selectLatestSniff();
            socket.send(JSON.stringify({status: "success", record: latest }))
            break;
          }

          default:
            socket.send(
              JSON.stringify({ error: `Unknown command: ${command}` }),
            );
        }
      } catch (err) {
        console.error("WS Error:", err);
        socket.send(JSON.stringify({ error: "Failed to process message" }));
      }

    };
    socket.onclose = () => clients.delete(socket);
    socket.onerror = () => clients.delete(socket);

    return response;
  }
  //==============================
  // ROUTES
  //==============================
  console.log("HTTP Request");

  // GET /sniffs (selectAll -> Sniff[])
  if (req.method === "GET" && url.pathname === "/sniffs") {
    console.log("GET /sniffs");
    try {
      const records: Sniff[] = await selectAllSniffs();
      return Response.json(records, { status: 200 });
    } catch (err) {
      console.error("HTTP Select Error:", err);
      return Response.json({ error: "Database retrieval failed" }, {
        status: 500,
      });
    }
  }
  // POST /sniffs (insert)
  if (req.method === "POST" && url.pathname === "/sniffs") {
    console.log("POST /sniffs");
    try {
      const body = await req.json().catch(() => null);
      if (!isValidPayload(body)) {
        return Response.json({ error: "Invalid payload format" }, {
          status: 400,
        });
      }
      const record = await insertSniff(body);
      return Response.json(record, { status: 201 });
    } catch (err) {
      console.error("HTTP Insert Error", err);
      return Response.json({ error: "Database insertion failed" }, {
        status: 500,
      });
    }
  }
  // POST /sniffs (insertMany)
  if (req.method === "POST" && url.pathname === "/sniffs/batch") {
    console.log("POST /sniffs/batch");
    try {
      const body = await req.json().catch(() => null);
      if (!isValidPayloadArray(body)) {
        return Response.json({
          error: "Invalid payload format, expected non-empty array",
        }, { status: 400 });
      }
      const records: Sniff[] = await insertManySniffs(body);
      return Response.json(records, { status: 201 });
    } catch (err) {
      console.log("HTTP InsertMany Error:", err);
      return Response.json({ error: "Database batch insertion failed" }, {
        status: 500,
      });
    }
  }

  // GET /sniffs (selectAll -> Sniff[])
  if (req.method === "GET" && url.pathname === "/") {
        return new Response(html, { status: 200, headers: {"content-type": "text/html; charset=utf-8"},});
  }
  return new Response("Not Found", { status: 404 });
});

const html = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Live Sniff Monitor</title>
  <style>
    body { font-family: system-ui, -apple-system, sans-serif; background: #0f172a; color: #f8fafc; margin: 0; padding: 2rem; }
    .container { max-width: 600px; margin: 0 auto; background: #1e293b; padding: 2rem; border-radius: 12px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
    .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #334155; padding-bottom: 1rem; }
    .badge { padding: 0.25rem 0.75rem; border-radius: 9999px; font-size: 0.875rem; font-weight: bold; }
    .online { background: #059669; color: #ecfdf5; }
    .offline { background: #dc2626; color: #fef2f2; }
    .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 1rem; margin: 1.5rem 0; }
    .card { background: #334155; padding: 1rem; border-radius: 8px; text-align: center; }
    .val { font-size: 1.75rem; font-weight: bold; margin-top: 0.5rem; color: #38bdf8; }
    .raw { background: #0f172a; padding: 1rem; border-radius: 8px; font-family: monospace; word-break: break-all; margin-top: 1rem; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h2>Latest Sniff Reading</h2>
      <span id="status" class="badge offline">Disconnected</span>
    </div>
    
    <div class="grid">
      <div class="card">
        <div>CO2 (ppm)</div>
        <div id="co2" class="val">--</div>
      </div>
      <div class="card">
        <div>Humidity (%)</div>
        <div id="humidity" class="val">--</div>
      </div>
      <div class="card">
        <div>Temperature (°C)</div>
        <div id="temp" class="val">--</div>
      </div>
    </div>

    <div><strong>Data / Note:</strong></div>
    <div id="data" class="raw">Waiting for updates...</div>
    <p style="font-size: 0.8rem; color: #94a3b8; text-align: right;">
      ID: <span id="sniff_id">--</span> | Created: <span id="created_at">--</span>
    </p>
  </div>

  <script>
    const statusEl = document.getElementById("status");
    const co2El = document.getElementById("co2");
    const humEl = document.getElementById("humidity");
    const tempEl = document.getElementById("temp");
    const dataEl = document.getElementById("data");
    const idEl = document.getElementById("sniff_id");
    const timeEl = document.getElementById("created_at");

    function connect() {
      const protocol = location.protocol === "https:" ? "wss:" : "ws:";
      const ws = new WebSocket(protocol+location.host);

      ws.onopen = () => {
        console.log("WebSocket connected!");
        statusEl.textContent = "Live";
        statusEl.className = "badge online";
      };

      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          if (msg.type === "latest_sniff" && msg.data) {
            const s = msg.data;
            co2El.textContent = s.co2;
            humEl.textContent = s.humidity;
            tempEl.textContent = s.temperature;
            dataEl.textContent = s.data;
            idEl.textContent = s.sniff_id;
            timeEl.textContent = new Date(s.created_at).toLocaleString();
          }
        } catch (e) {
          console.error(e);
        }
      };

      ws.onclose = () => {
        statusEl.textContent = "Disconnected";
        statusEl.className = "badge offline";
        setTimeout(connect, 3000);
      };
    }

    connect();
  </script>
</body>
</html>`;
