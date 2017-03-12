const http = require("http")
const TCP = process.binding("tcp_wrap").TCP
const WebServer = require('bindings')("native-static-webserver")
const fs = require("fs")


let server = http.createServer(function(req,res){
    //req.socket._handle.unref()
    var content  = fs.readFileSync("index.html");
    if( req.url === "/1" )
    {
        res.write(content.toString())
        res.end();
    }
    else
    {
        WebServer.forward(req.socket._handle,content.toString())
    }
    

    
    /**/
    //res.write(" Hello From static - server");
    console.log('from main thread')    
    //res.end();
});


server.listen(8001)