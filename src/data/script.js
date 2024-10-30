const ws = new WebSocket(`ws://${window.location.hostname}/ws`);

function receiveStream() {
  const url = `http://${window.location.hostname}/stream`;

  fetch(url)
    .then(response => {
      const reader = response.body.getReader();
      let imageData = "";

      function readStream() {
        reader.read().then(({ done, value }) => {
          if (done) return;

          imageData += new TextDecoder().decode(value);

          const jpegStart = imageData.indexOf("\r\n\r\n");
          if (jpegStart !== -1) {
            const imageBlob = new Blob([imageData.substring(jpegStart + 4)], { type: "image/jpeg" });
            const imageUrl = URL.createObjectURL(imageBlob);

            document.getElementById("stream").src = imageUrl;

            imageData = "";
          }

          readStream();
        });
      }

      readStream();
    })
    .catch(error => {
      console.error("Error al recibir el stream:", error);
    });
}

receiveStream();

document.addEventListener("keydown", (event) => {
  const key = event.key.toLowerCase();

  if (["w", "a", "s", "d"].includes(key)) {
    ws.send(key);
    console.log("Tecla enviada:", key);
  }
});

ws.onmessage = function (event) {
  console.log("Mensaje recibido del servidor:", event.data);
};

ws.onerror = function (error) {
  console.error("Error en WebSocket:", error);
};

ws.onopen = function () {
  console.log("Conexión WebSocket establecida");
};

ws.onclose = function () {
  console.log("Conexión WebSocket cerrada");
};