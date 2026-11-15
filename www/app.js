document.getElementById("boton").addEventListener("click", saludarCliente);

function saludarCliente(){
    const saludo = "Hola cliente";
    document.getElementById("prueba").innerText = saludo;
}
