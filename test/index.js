import RPCClient from "../lib/RPCClient.js"

var cli = new RPCClient({ client_id: "", client_secret: "" })

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
      instance: true
    }
  })
  //cli.authorize(["identify"], "https://ten-lace-meteorite.glitch.me")
})
await cli.connect()
console.log("ready")