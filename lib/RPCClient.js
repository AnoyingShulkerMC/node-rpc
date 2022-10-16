import { EventEmitter, once } from "node:events"
import { createRequire } from "node:module"
import fetch from "node-fetch"
const { BaseConnection, Register } = createRequire(import.meta.url)("../build/Release/node-rpc")
console.log(BaseConnection)
class RPCError extends Error {
  constructor(message, code, command) {
    super(message)
    this.name = `RPCError[${code}]`
    this.command = command
  }
}
class RPCClient extends EventEmitter {
  pollingTimer = null
  static Opcodes = {
    Handshake: 0,
    Frame: 1,
    Close: 2, 
    Ping: 3,
    Pong: 4
  }
  con = null
  constructor({ client_id, client_secret } = {}) {
    super()
    /**
     * @type {BaseConnection}
     * @private
     */
    this.con = new BaseConnection()
    /** @private */
    this.client_id = "" + client_id
    /** 
     *  @type {Map<string, Array<Function>>} 
     *  @private
     */
    this.commands = new Map()
    this.client_secret = null || client_secret
  }
  async connect(pipe) {
    this.con.open(pipe);
    this.con.write(0, JSON.stringify({v: 1, client_id: this.client_id}))
    this.pollingTimer = setInterval(this.handleMessage.bind(this), 1000)
    await once(this, "READY")
  }
  close() {
    clearInterval(this.pollingTimer)
    this.con.close()
  }
  debug(msg) {
    this.emit("debug", msg)
  }
  handleMessage() {
    var msg = this.con.read();
    if (msg == null) return;
    this.debug(`[MESSAGE] Opcode: ${msg.opcode}
    Message: ${msg.message}`)
    switch (msg.opcode) {
      case RPCClient.Opcodes.Frame:
        this.handlePacket(JSON.parse(msg.message))
        break;
      case RPCClient.Opcodes.Close:
        var code = JSON.parse(msg.message).code
        if (code == 1000) {
          this.close()
          this.connect()
        }
        break;
      case RPCClient.Opcodes.Ping:
        this.con.write(RPCClient.Opcodes.Pong, msg.message)
        break
      case RPCClient.Opcodes.Pong:
        break;
      default:
        this.con.close()
    }
  }
  /**
   * 
   * @param {RPCPayload} msg
   */
  handlePacket(msg) {
    this.debug(`[FRAME]
    CMD:  ${msg.cmd}
    EVT:  ${msg.evt}
    DATA: ${JSON.stringify(msg.data)}`)
    if (msg.cmd == "DISPATCH") return this.emit(msg.evt, msg.data)
    if (!this.commands.has(msg.nonce)) return;
    var [resolve, reject] = this.commands.get(msg.nonce)
    if (msg.evt == "ERROR") {
      reject(new RPCError(msg.data.message,msg.data.code, msg.cmd))
    } else {
      resolve(msg.data)
    }
    this.commands.delete(msg.nonce)
  }
  execCommand(cmd, args, addPayload = {}) {
    return new Promise((resolve, reject) => {
      this.debug(`[EXEC_CMD]
    Command:   ${cmd}
    Arguments: ${JSON.stringify(args)}`)
      const nonce = Date.now()
      this.commands.set(nonce, [resolve, reject])
      var payload = {
        cmd,
        args,
        nonce,
        ...addPayload
      }
      this.con.write(RPCClient.Opcodes.Frame, JSON.stringify(payload))
    })
  }
  /**
   * 
   * @param {Array<string>} scopes
   * @param {string} redirect_url
   */
  async authorize(scopes, redirect_url) {
    if (this.client_secret == null) throw new Error("A client secret must be set")
    var code = (await this.execCommand("AUTHORIZE", { client_id: this.client_id, scopes })).code
    var access_token = await (await fetch("https://discord.com/api/v10/oauth2/token", {
      method: "POST",
      headers: {
        "content-type": "application/x-www-form-urlencoded"
      },
      body: `client_id=${this.client_id}&client_secret=${this.client_secret}&grant_type=authorization_code&code=${code}&redirect_uri=${encodeURIComponent(redirect_url)}`
    })).json()
    return await this.execCommand("AUTHENTICATE", {access_token: access_token.access_token})
  }
  subscribe(evt, args) {
    return this.execCommand("SUBSCRIBE", args, {evt})
  }
}
/**
 * @typedef {Object} BaseConnection
 * @property {Function} open Opens the connection
 * @property {Function} close Closes the connection
 * @property {Function} write Writes to the connection
 * @property {Function} read Reads any incoming messages. Returns null if there isn't any. Otherwise, returns in the format {opcode, message}
 */
/**
 * @typedef {Object} RPCPayload
 * @property {string} cmd The command to execute. If recieved, the command the server is responding to.
 * @property {string} nonce Used for storing replies from the server,
 * @property {string} evt Event dispatched from the server
 * @property {object} data Event data
 * @property {object} args The arguments for the command
 */
export default RPCClient
export { Register }