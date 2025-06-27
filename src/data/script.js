let socket = new WebSocket("ws://" + window.location.host + ":81");

socket.onopen = function () {
    console.log("✅ WebSocket conectado!");
};

socket.onerror = function (error) {
    console.error("❌ WebSocket error:", error);
};

socket.onclose = function () {
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

// Teclado
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

// Gamepad
let gamepadIndex = null;
let lastSentCommand = "";

window.addEventListener("gamepadconnected", (e) => {
    console.log("🎮 Control conectado:", e.gamepad);
    gamepadIndex = e.gamepad.index;
});

window.addEventListener("gamepaddisconnected", () => {
    console.log("🎮 Control desconectado");
    gamepadIndex = null;
});

function pollGamepad() {
    if (gamepadIndex === null) return;

    const gp = navigator.getGamepads()[gamepadIndex];
    if (!gp) return;

    const dpadUp = gp.buttons[12].pressed;
    const dpadDown = gp.buttons[13].pressed;
    const dpadLeft = gp.buttons[14].pressed;
    const dpadRight = gp.buttons[15].pressed;

    let command = "";
    if (dpadUp) command = "forward";
    else if (dpadDown) command = "backward";
    else if (dpadLeft) command = "left";
    else if (dpadRight) command = "right";
    else command = "stop";

    if (command !== lastSentCommand) {
        sendCommand(command);
        lastSentCommand = command;
    }
}

// Llamar periódicamente
setInterval(pollGamepad, 100); // cada 100ms
