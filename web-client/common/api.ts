// File describes the API between the web client and the server
// It is used to exhange data over websocket.

export enum ItemType {
  ASTEROID = "asteroid",
  SHIP = "ship",
  UNKNOWN = "unknown"
}

// [x, y, vx, vy]
export type Position = [number, number, number, number]

export type Item = {
  type: ItemType,
  id: number,
  ts: number,
  pos: Position,
  radius?: number
}

export type Update = {
  ts: number,
  items: Item[]
}