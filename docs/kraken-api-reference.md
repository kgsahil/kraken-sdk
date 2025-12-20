# Kraken WebSocket API v2 Reference

This document captures the actual Kraken WebSocket API v2 specification based on official documentation.

---

## Connection Endpoints

| Type | Endpoint | Authentication |
|------|----------|----------------|
| **Public** | `wss://ws.kraken.com/v2` | None required |
| **Private** | `wss://ws-auth.kraken.com/v2` | Token required |
| **Beta Public** | `wss://beta-ws.kraken.com/v2` | None required |
| **Beta Private** | `wss://beta-ws-auth.kraken.com/v2` | Token required |

**Requirements:**
- TLS with Server Name Indication (SNI)
- JSON message encoding
- Connection timeout: ~1 minute of inactivity (send pings to keep alive)

---

## Message Structure

### Subscribe Request

```json
{
  "event": "subscribe",
  "subscription": {
    "name": "ticker",
    "depth": 10  // Optional, for book channel
  },
  "pair": ["BTC/USD", "ETH/USD"],
  "req_id": 123  // Optional, for response matching
}
```

**Fields:**
- `event`: Always `"subscribe"` for subscriptions
- `subscription.name`: Channel name (`ticker`, `book`, `trade`, `ohlc`, etc.)
- `subscription.depth`: Optional, for book channel (10, 25, 100, 500, 1000)
- `subscription.token`: Required for private channels (obtained via REST API)
- `pair`: Array of trading pairs in "BTC/USD" format
- `req_id`: Optional integer for correlating requests with responses

### Unsubscribe Request

```json
{
  "event": "unsubscribe",
  "subscription": {
    "name": "ticker"
  },
  "pair": ["BTC/USD"],
  "req_id": 124
}
```

### Response Format

```json
{
  "time_in": "2021-05-11T19:47:09.896860Z",
  "time_out": "2021-05-11T19:47:09.897123Z",
  "success": true,
  "result": {
    "channelID": 123,
    "channelName": "ticker",
    "pair": "BTC/USD"
  },
  "error": null,
  "req_id": 123  // Matches request if provided
}
```

**Fields:**
- `time_in`: RFC3339 timestamp when request received
- `time_out`: RFC3339 timestamp when response sent
- `success`: Boolean indicating if subscription succeeded
- `result`: Object with `channelID`, `channelName`, `pair` on success
- `error`: Error message string if `success` is false
- `req_id`: Echoes request `req_id` if provided

---

## Public Channels

### 1. Ticker (`ticker`)

**Subscribe:**
```json
{
  "event": "subscribe",
  "subscription": {"name": "ticker"},
  "pair": ["BTC/USD"]
}
```

**Update Message:**
```json
[
  123,  // channelID
  {
    "a": ["50000.00", "1.5", "1.5"],      // ask [price, whole_lot_volume, lot_volume]
    "b": ["49900.00", "2.0", "2.0"],      // bid [price, whole_lot_volume, lot_volume]
    "c": ["50000.00", "0.1"],             // last trade [price, volume]
    "v": ["1500.5", "2000.3"],            // volume [today, last 24 hours]
    "p": ["49950.00", "49980.00"],        // volume weighted average price [today, last 24 hours]
    "t": [100, 200],                       // number of trades [today, last 24 hours]
    "l": ["49000.00", "48000.00"],        // low [today, last 24 hours]
    "h": ["51000.00", "52000.00"],        // high [today, last 24 hours]
    "o": "49500.00"                        // today's opening price
  },
  "ticker",
  "BTC/USD"
]
```

### 2. Book (`book`)

**Subscribe:**
```json
{
  "event": "subscribe",
  "subscription": {
    "name": "book",
    "depth": 10
  },
  "pair": ["BTC/USD"]
}
```

**Snapshot (First Message):**
```json
[
  456,
  {
    "as": [["50000.00", "1.5", "1234567890"]],  // asks [price, volume, timestamp]
    "bs": [["49900.00", "2.0", "1234567890"]],  // bids [price, volume, timestamp]
    "checksum": "1234567890"  // CRC32 checksum of top 10 levels
  },
  "book-10",
  "BTC/USD"
]
```

**Update Message:**
```json
[
  456,
  {
    "a": [["50001.00", "1.0", "1234567891"]],  // ask updates
    "b": [["49901.00", "1.5", "1234567891"]],  // bid updates
    "c": "1234567891"  // checksum
  },
  "book-10",
  "BTC/USD"
]
```

**Checksum:**
- CRC32 of top 10 bid/ask levels
- Format: Price + Volume pairs, sorted by price
- Validates data integrity

### 3. Trade (`trade`)

**Subscribe:**
```json
{
  "event": "subscribe",
  "subscription": {"name": "trade"},
  "pair": ["BTC/USD"]
}
```

**Update Message:**
```json
[
  789,
  [
    ["50000.00", "0.1", "1234567890.123", "b", "l", ""],  // [price, volume, time, side, order_type, misc]
    ["50001.00", "0.2", "1234567890.456", "s", "m", ""]
  ],
  "trade",
  "BTC/USD"
]
```

### 4. OHLC (`ohlc`)

**Subscribe:**
```json
{
  "event": "subscribe",
  "subscription": {
    "name": "ohlc",
    "interval": 60  // 1, 5, 15, 30, 60, 240, 1440, 10080, 21600
  },
  "pair": ["BTC/USD"]
}
```

---

## Trading Operations

Kraken WebSocket API v2 supports trading operations via the authenticated endpoint.

### Add Order

**Request:**
```json
{
  "event": "addOrder",
  "token": "your_authentication_token",
  "req_id": 123,
  "pair": "BTC/USD",
  "type": "buy",
  "ordertype": "limit",
  "price": "50000.00",
  "volume": "0.1",
  "userref": "my-order-123",
  "oflags": "post"  // Optional: "post" for post-only, "fcib" for fee-in-base, etc.
}
```

**Response:**
```json
{
  "time_in": "2021-05-11T19:47:09.896860Z",
  "time_out": "2021-05-11T19:47:09.897123Z",
  "success": true,
  "result": {
    "txid": ["O12345-67890-ABCDE"],
    "descr": {
      "order": "buy 0.10000000 BTC/USD @ limit 50000.00"
    }
  },
  "error": null,
  "req_id": 123
}
```

### Cancel Order

**Request:**
```json
{
  "event": "cancelOrder",
  "token": "your_authentication_token",
  "req_id": 124,
  "txid": "O12345-67890-ABCDE"
}
```

### Cancel All Orders

**Request:**
```json
{
  "event": "cancelAll",
  "token": "your_authentication_token",
  "req_id": 125
}
```

### Cancel All Orders After

**Request:**
```json
{
  "event": "cancelAllOrdersAfter",
  "token": "your_authentication_token",
  "req_id": 126,
  "timeout": 60  // Seconds
}
```

---

## Private Channels

### Authentication

1. **Get Token via REST API:**
   ```
   POST https://api.kraken.com/0/private/GetWebSocketsToken
   ```
   Response:
   ```json
   {
     "error": [],
     "result": {
       "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
       "expires": 900  // seconds until expiration
     }
   }
   ```

2. **Use Token in Subscription:**
   ```json
   {
     "event": "subscribe",
     "subscription": {
       "name": "ownTrades",
       "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
     }
   }
   ```

**Token Validity:**
- Valid for 15 minutes after creation
- Does not expire while WebSocket connection is active
- Must be used within 15 minutes of creation

### Private Channels

1. **openOrders** - User's open orders with real-time updates
2. **ownTrades** - User's last 50 trades + real-time updates
3. **balance** - Account balances and ledger transactions

---

## Key Design Principles

### 1. FIX-Like Structure
- Consistent message format
- Standardized field names
- Predictable response structure

### 2. Symbol Format
- Always "BTC/USD" format (not "XBTUSD" or "XXBTZUSD")
- Forward slash separator
- Base/Quote order

### 3. Timestamps
- RFC3339 format: `2021-05-11T19:47:09.896860Z`
- Microsecond precision
- UTC timezone

### 4. Data Types
- Prices/quantities: Numbers (not strings)
- Maintains full precision
- No scientific notation

### 5. Request ID (`req_id`)
- Optional integer field
- Echoed in response
- Enables request/response correlation

---

## Rate Limits

- **Connection/Reconnection:** ~150 attempts per rolling 10 minutes per IP (Cloudflare limit)
- **Subscription Requests:** 1 request/second (default, may vary with API tier)
- **Data Reception:** No limit (push-based, continuous stream)

---

## Error Handling

### Connection Errors
- Network failures: Reconnect with exponential backoff
- Authentication failures: Re-fetch token
- Rate limit exceeded: Wait and retry

### Message Errors
```json
{
  "success": false,
  "error": "Subscription depth not supported",
  "req_id": 123
}
```

### Common Errors
- Invalid pair format
- Unsupported channel
- Invalid depth (for book channel)
- Missing token (for private channels)
- Expired token

---

## Best Practices

1. **Always use `req_id`** for subscription requests to track confirmation
2. **Handle checksums** on order book updates to detect missed messages
3. **Send periodic pings** to keep connection alive (~30 second intervals)
4. **Implement exponential backoff** for reconnections
5. **Validate timestamps** - don't use as unique identifiers
6. **Respect rate limits** - queue subscription requests if needed

---

## References

- [Kraken WebSocket API v2 Documentation](https://docs.kraken.com/websockets-v2/)
- [Kraken API Center](https://docs.kraken.com/api/)
- [WebSocket API v2 Guide](https://docs.kraken.com/api/docs/guides/spot-ws-intro/)

