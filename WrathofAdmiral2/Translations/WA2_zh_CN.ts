<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name></name>
    <message id="catbomb">
        <location filename="../Client/client.cpp" line="82"/>
        <source>You have been bombarded by a cute cat.</source>
        <translation type="unfinished">服务器给你扔了一只可爱的猫娘。</translation>
    </message>
    <message id="login-success">
        <location filename="../Client/client.cpp" line="305"/>
        <source>%1: Login success</source>
        <translation type="unfinished">%1:登录成功</translation>
    </message>
    <message id="connection-failed-warning">
        <location filename="../Client/client.cpp" line="96"/>
        <source>Failed to establish connection, check your username, password and server status.</source>
        <oldsource>Failed to establish connection, check your username,password and server status.</oldsource>
        <translation type="unfinished">连接服务器失败，可能是用户名、密码不对或者服务器未在工作。</translation>
    </message>
    <message id="fscktanaka">
        <location filename="../Client/client.cpp" line="107"/>
        <source>田中飞妈</source>
        <translation type="unfinished">田中飞妈</translation>
    </message>
    <message id="password-mismatch">
        <location filename="../Client/client.cpp" line="135"/>
        <source></source>
        <oldsource>Password does not match!</oldsource>
        <translation type="unfinished">两次输入密码不一致！</translation>
    </message>
    <message id="wait-for-connect-failure">
        <location filename="../Client/client.cpp" line="159"/>
        <source>Failed to connect to server at %1:%2</source>
        <translation type="unfinished">未能连接到服务器，位于IP %1 端口 %2</translation>
    </message>
    <message id="password-confirm">
        <location filename="../Client/client.cpp" line="169"/>
        <source></source>
        <oldsource>Confirm Password:</oldsource>
        <translation type="unfinished">密码确认：</translation>
    </message>
    <message id="register-usage">
        <location filename="../Client/client.cpp" line="208"/>
        <source>Usage: register [ip] [port] [username]</source>
        <translation type="unfinished">用法：register [IP] [端口] [用户名]</translation>
    </message>
    <message id="connect-usage">
        <location filename="../Client/client.cpp" line="213"/>
        <source>Usage: connect [ip] [port] [username]</source>
        <translation type="unfinished">用法：connect [IP] [端口] [用户名]</translation>
    </message>
    <message id="connected-already">
        <location filename="../Client/client.cpp" line="194"/>
        <source></source>
        <oldsource>Already connected, please shut down first.</oldsource>
        <translation type="unfinished">已在连接中，请先下线。</translation>
    </message>
    <message id="connect-duplicate">
        <location filename="../Client/client.cpp" line="199"/>
        <source></source>
        <oldsource>Do not attempt duplicate connections!</oldsource>
        <translation type="unfinished">不要重复尝试连接！</translation>
    </message>
    <message id="ip-invalid">
        <location filename="../Client/client.cpp" line="223"/>
        <location filename="../Server/server.cpp" line="392"/>
        <source>IP isn&apos;t valid</source>
        <translation type="unfinished">IP格式不正确</translation>
    </message>
    <message id="port-invalid">
        <location filename="../Client/client.cpp" line="230"/>
        <location filename="../Server/server.cpp" line="399"/>
        <source>Port isn&apos;t valid, it must fall between 1024 and 49151</source>
        <translation type="unfinished">端口格式不正确，注意1024以下和49151以上的端口是不能任意使用的</translation>
    </message>
    <message id="password-enter">
        <location filename="../Client/client.cpp" line="236"/>
        <source></source>
        <oldsource>Enter Password:</oldsource>
        <translation type="unfinished">输入密码：</translation>
    </message>
    <message id="disconnect-when-offline">
        <location filename="../Client/client.cpp" line="245"/>
        <source></source>
        <oldsource>Not under a valid connection.</oldsource>
        <translation type="unfinished">未在有效连接中。</translation>
    </message>
</context>
<context>
    <name>Client</name>
    <message id="logout-failed">
        <location filename="../Client/client.cpp" line="253"/>
        <source>%1: failed to send logout attmpt - %2</source>
        <translation type="unfinished"></translation>
    </message>
    <message id="register-success">
        <location filename="../Client/client.cpp" line="281"/>
        <source>%1: Register success</source>
        <translation type="unfinished"></translation>
    </message>
    <message id="disconnect-attempt">
        <location filename="../Client/client.cpp" line="257"/>
        <source>Attempting to disconnect...</source>
        <translation>试图下线…</translation>
    </message>
    <message id="register-failed">
        <location filename="../Client/client.cpp" line="295"/>
        <location filename="../Client/client.cpp" line="492"/>
        <source>%1: register failure, reason: %2</source>
        <translation>%1:注册失败，理由:%2</translation>
    </message>
    <message id="login-failed">
        <location filename="../Client/client.cpp" line="320"/>
        <location filename="../Client/client.cpp" line="503"/>
        <source>%1: Login failure, reason: %2</source>
        <translation>%1:登录失败，理由:%2</translation>
    </message>
    <message id="logout-success">
        <location filename="../Client/client.cpp" line="332"/>
        <source>%1: Logout success</source>
        <oldsource>%1: Logout success:</oldsource>
        <translation>%1:下线成功</translation>
    </message>
    <message id="logout-notonline">
        <location filename="../Client/client.cpp" line="347"/>
        <source>%1: Logout failure, not online</source>
        <oldsource>%1: Logout failure, not online:</oldsource>
        <translation>%1:登录失败，不在线上</translation>
    </message>
    <message id="retransmit-toomuch">
        <location filename="../Client/client.cpp" line="405"/>
        <source>%1: max restransmit time exceeded!</source>
        <translation>%1:重新传输次数超过限制!</translation>
    </message>
    <message id="sent-login-failed">
        <source>%1: login failed - %2</source>
        <translation type="obsolete">%1:登录信息发送失败，错误:%2</translation>
    </message>
    <message id="disconnect-attemp">
        <source>Attempting to disconnect...</source>
        <translation type="vanished">试图下线…</translation>
    </message>
    <message id="malformed-shadow">
        <location filename="../Client/client.cpp" line="289"/>
        <location filename="../Client/client.cpp" line="314"/>
        <source>Malformed shadow</source>
        <translation>密文格式不对</translation>
    </message>
    <message id="user-exists">
        <location filename="../Client/client.cpp" line="291"/>
        <source>User Exists</source>
        <translation>用户已存在</translation>
    </message>
    <message id="password-incorrect">
        <location filename="../Client/client.cpp" line="316"/>
        <source>Password incorrect</source>
        <translation>密码错误</translation>
    </message>
    <message id="logout-forced">
        <location filename="../Client/client.cpp" line="337"/>
        <source>%1: Logged elsewhere, force quitting</source>
        <translation>用户%1在别处登录，已强制下线</translation>
    </message>
    <message id="client-bad-json">
        <location filename="../Client/client.cpp" line="361"/>
        <source>Client sent a bad json</source>
        <translation>客户端发送了不正确的JSON</translation>
    </message>
    <message id="client-unsupported-json">
        <location filename="../Client/client.cpp" line="363"/>
        <source>Client sent nsupported message format</source>
        <translation>客户端发送了不支持的信息</translation>
    </message>
    <message id="remote-disconnect">
        <location filename="../Client/client.cpp" line="465"/>
        <source>Remote disconnected.</source>
        <translation>远程连接断开。</translation>
    </message>
</context>
<context>
    <name>CommandLine</name>
    <message id="license-not-found">
        <location filename="../Protocol/commandline.cpp" line="146"/>
        <source>Can&apos;t find license file, exiting.</source>
        <translation>找不到许可证文件，退出。</translation>
    </message>
    <message id="naganami">
        <location filename="../Protocol/commandline.cpp" line="162"/>
        <source>What? Admiral Tanaka? He&apos;s the real deal, isn&apos;t he?
Great at battle and bad at politics--so cool!</source>
        <translation type="unfinished"></translation>
    </message>
    <message id="invalid-command">
        <location filename="../Protocol/commandline.cpp" line="256"/>
        <source>Invalid Command, use &apos;commands&apos; for valid commands, &apos;help&apos; for help, &apos;exit&apos; to exit.</source>
        <translation>命令不存在，请使用&quot;commands&quot;查看可用命令，&quot;help&quot;查看帮助，&quot;exit&quot;退出。</translation>
    </message>
    <message id="exit-helper">
        <location filename="../Protocol/commandline.cpp" line="262"/>
        <source>Use &apos;exit&apos; to quit.</source>
        <translation>若要退出请使用&quot;exit&quot;。</translation>
    </message>
    <message id="good-command">
        <location filename="../Protocol/commandline.cpp" line="266"/>
        <source>Available commands:</source>
        <translation>可用命令：</translation>
    </message>
    <message id="all-command">
        <location filename="../Protocol/commandline.cpp" line="272"/>
        <source>All commands:</source>
        <translation>全部命令：</translation>
    </message>
    <message id="help">
        <location filename="../Protocol/commandline.cpp" line="280"/>
        <source>Use &apos;exit&apos; to quit, &apos;help&apos; to show help, &apos;commands&apos; to show available commands.</source>
        <translation>请使用&quot;commands&quot;查看可用命令，&quot;help&quot;查看帮助，&quot;exit&quot;退出。</translation>
    </message>
    <message id="goodbye">
        <location filename="../Protocol/commandline.cpp" line="416"/>
        <source>Goodbye, press ENTER to quit</source>
        <translation>再见，按ENTER退出程序</translation>
    </message>
</context>
<context>
    <name>Server</name>
    <message>
        <location filename="../Server/server.cpp" line="69"/>
        <source>Session cipher: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="72"/>
        <source>; session protocol: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="80"/>
        <source>DTLS 1.2.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="83"/>
        <source>DTLS 1.2 or later.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="86"/>
        <source>Unknown protocol.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="317"/>
        <source>SQL connection successful!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="322"/>
        <source>User database does not exist, creating...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="338"/>
        <source>User Database is OK.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="385"/>
        <source>Usage: listen [ip] [port]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="403"/>
        <source>Server is listening on address %1 and port %2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="409"/>
        <source>Server failed to listen on address %1 and port %2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="419"/>
        <source>Server stopped listening.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="424"/>
        <source>Server isn&apos;t listening.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="443"/>
        <source>Spurious read notification?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="453"/>
        <source>Failed to read a datagram:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="460"/>
        <source>Failed to extract peer info (address, port)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="479"/>
        <source>disconnected abruptly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="495"/>
        <source>PSK callback, received a client&apos;s identity: &apos;%1&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="528"/>
        <source>0 byte dgram, could be a re-connect attempt?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="546"/>
        <source>: handshake is in progress ...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="549"/>
        <source>Connection with %1 encrypted. %2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="559"/>
        <source>Server is shutting down</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="591"/>
        <source>: verified, starting a handshake</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="601"/>
        <source>DTLS error:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="603"/>
        <source>: not verified yet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="618"/>
        <source>Wait for disconnection...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../Server/server.cpp" line="622"/>
        <source>Disconnect failed!</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>
