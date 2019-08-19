var dgram = require('dgram'); // 引入 UDP/Datagram 模組
var HOST = '192.168.0.101';
var PORT = 5683;


console.info('Now create UDP Server...');
// 建立 UDP 伺服器
// 使用 dgram.createSocket() 方法建立一個 UDP 伺服器
// udp4 類型的資料通訊端，對應到的是 IPv4 協定；另有 udp6，對應的是 IPv6 協定
var server = dgram.createSocket('udp4');

// 為 UDP 伺服器新增一個 listening 事件函式
server.on('listening', function(){
  var address = server.address(); // 取得伺服器的位址和port
  console.log('UDP Server listening on ' + address.address + ':' + address.port);
});

// 為 UDP 伺服器新增一個 message 事件函式
server.on('message', function(message, remote){
  console.log('UDP Server received from ' + remote.address + ':' + remote.port);
  console.log(' - ' + message);
  //server.close();
});

// 為 UDP 伺服器新增一個 error 事件函式
server.on('error', function(err){
  console.log('Server error:\n' + err.stack);
  server.close();
});

// 為 UDP 伺服器新增一個 close 事件函式
server.on('close', function(err){
  console.log('Server closed');
});

// 為 UDP 伺服器鎖定主機和 port
server.bind(PORT, HOST);