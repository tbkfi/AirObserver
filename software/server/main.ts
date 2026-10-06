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
// FUNCTIONS
//==============================

async function insertSniff({ co2, humidity, temperature, data }: SniffPayload): Promise<Sniff> {
  const query =
    `INSERT INTO sniffs (co2, humidity, temperature, data) VALUES ($1,$2,$3, $4) RETURNING sniff_id, co2, humidity, temperature, data, created_at;`;
  const result = await pool.query(query, [co2, humidity, temperature, data]);
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
  const query = `INSERT INTO sniffs (co2, humidity, temperature, data) VALUES ${
    valuePlaceholders.join(", ")
  } RETURNING sniff_id, co2, humidity, temperature, data, created_at;`;
  const result = await pool.query(query, params);
  return result.rows;
}


async function selectAllSniffs(): Promise<Sniff[]> {
  const query =
    `SELECT sniff_id, co2, humidity, temperature, data, created_at FROM sniffs ORDER BY created_at DESC;`;
  const result = await pool.query(query);
  return result.rows;
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

          default:
            socket.send(
              JSON.stringify({ error: `Unknown command: ${command}` }),
            );
        }
      } catch (err) {
        console.error("WS Error:", err);
        socket.send(JSON.stringify({ error: "Failed to process message" }));
      }
      return response;
    };
  }
  //==============================
  // ROUTES
  //==============================
  console.log("HTTP Request");

  // GET /sniffs (selectAll -> Sniff[])
  if (req.method === "GET" && url.pathname === "/test") {
    console.log("GET /test");
    return Response.json({}, { status: 200 });
  }
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
  return new Response("Not Found", { status: 404 });
});
