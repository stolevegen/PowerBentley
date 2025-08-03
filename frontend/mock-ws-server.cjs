// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

const { generateMockResponse, getFakeSpeed } = require('./src/lib/mockdata.js')

// Simple mock WebSocket server for frontend development
const WebSocket = require('ws')
const wss = new WebSocket.Server({ port: 8080 })

wss.on('connection', (ws) => {
  ws.on('message', (msg) => {
    let data
    try {
      data = JSON.parse(msg)
    } catch (e) {
      return
    }
    setTimeout(() => {
      const mockResponses = generateMockResponse(data)

      mockResponses.forEach((response) => {
        ws.send(JSON.stringify(response))
      })
    }, 500)

    setInterval(() => {
      ws.send(JSON.stringify({ type: 'read_speed_response', current_speed: getFakeSpeed() }))
    }, 300) // Send speed every 300ms
  })
})

console.log('Mock WebSocket server running on ws://localhost:8080')
