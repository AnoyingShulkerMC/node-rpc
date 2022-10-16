import RPCClient from "../lib/RPCClient.js"
import { Register } from "../lib/RPCClient.js"
import { fileURLToPath } from "node:url"
var cli = new RPCClient({ client_id: "1030631289973379133", client_secret: "XeuDqueKK1k_f5oZxFxgpFZQ5YvxdPld" })
console.log(process.argv)
cli.on("debug", console.log)
cli.on("READY", () => {
  cli.execCommand("SET_ACTIVITY", {
    pid: process.pid,
    activity: {
      details: "\u5927\u9e21\u9e21\u5f88\u5927",
      state: "yes",
      assets: {
        large_image: "easports",
        large_text: "yes",
        small_image: "easports",
        small_text: "YES"
      },
      instance: true,
      secrets: {
        join: "test"
      },
      party: {
        size: [2, 3],
        id: "yesss"
      }
    }
  })
  cli.subscribe("ACTIVITY_JOIN_REQUEST")
  cli.subscribe("ACTIVITY_JOIN")
  cli.on("ACTIVITY_JOIN_REQUEST", (d) => {
    cli.execCommand("SEND_ACTIVITY_JOIN_INVITE",{
      user_id: d.user.id
    })
  })
})

Register(cli.client_id, `"${process.execPath}" "${fileURLToPath(import.meta.url)}" "%1"`)
await cli.connect()
console.log("ready")