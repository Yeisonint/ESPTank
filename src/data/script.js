let socket = new WebSocket("ws://" + window.location.host + ":81");

socket.onopen = function() {
    console.log("✅ WebSocket conectado!");
};

socket.onerror = function(error) {
    console.error("❌ WebSocket error:", error);
};

socket.onclose = function() {
    console.warn("⚠️ WebSocket desconectado. Reconectando en 3 segundos...");
    setTimeout(() => location.reload(), 3000);
};

function sendCommand(cmd) {
    if (socket.readyState === WebSocket.OPEN) {
        socket.send(cmd);
    } else {
        console.warn("⏳ WebSocket no está listo. Estado:", socket.readyState);
    }
}

// Capturar teclas WASD para control de motores
document.addEventListener("keydown", (event) => {
    if (event.repeat) return;
    let command = "";
    if (event.key === "w") command = "forward";
    else if (event.key === "s") command = "backward";
    else if (event.key === "a") command = "left";
    else if (event.key === "d") command = "right";

    if (command) sendCommand(command);
});

document.addEventListener("keyup", () => sendCommand("stop"));
