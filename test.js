const http = require("http")
const TCP = process.binding("tcp_wrap").TCP
const WebServer = require('bindings')("native-static-webserver")


let server = http.createServer(function(req,res){
    
    WebServer.forward(req.socket._handle)
    console.log('from main thread')    
});


server.listen(8001)