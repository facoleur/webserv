async function sendGetRequest(url) {
  const response = await fetch(url, { method: "GET" });
  console.log("GET response:", response);
}

async function sendPostRequest(url) {
  const body = JSON.stringify({ key: "value" });

  const response = await fetch(url, {
    method: "POST",
    body,
  });

  console.log("POST response:", response);
}

async function sendChunkedPostRequest(url) {
  const encoder = new TextEncoder();
  const stream = new ReadableStream({
    start(controller) {
      controller.enqueue(encoder.encode('{"part1":"hello",'));
      setTimeout(() => {
        controller.enqueue(encoder.encode('"part2":"world"}'));
        controller.close();
      }, 1000);
    },
  });

  const response = await fetch(url, {
    method: "POST",
    duplex: "half",
    body: stream,
  });

  console.log("Chunked POST response:", response);
}

const url = "http://localhost:8080";

// sendGetRequest(url);
// sendPostRequest(url);
sendChunkedPostRequest(url);
