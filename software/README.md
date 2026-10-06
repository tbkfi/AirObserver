# Backend

## 1. Brief Summary

HTTP API and Web-Socket to insert data from the IoT device sensors via WiFi.

## 2.Technology Stack
The project relies on the listed technologies to run :
* Code :
    * Javascript
        * Typescript
* Database :
    * PostgreSQL
* DevOps methods :
    * Github
    * Docker
    * Deno
### Requirements:

* Docker CLI

## 3. How to run it?

### 3.1 Run the app using docker compose

1. Clone the repository:


```bash
git clone https://github.com/tbkfi/AirObserver.git
```

2. Move to folder software:

```bash
cd AirObserver/software
```

3. Run docker compile file:

```bash
docker compose -f docker/docker-compose.yml up --build

```

#### 3.2 Remove docker containers

1. Move to folder software:

```bash
cd AirObserver/software
```

2. docker compile command to remove containers:

```bash
docker compose -f docker/docker-compose.yml up down
```

## 4. How to format data

### 4.1 How to make API calls on terminal

* Get all stored data from sensor device

```bash
curl -X GET http://localhost:3000/sniffs \
  -H "Content-Type: application/json"
```

* Insert one data from sensor device into database

```bash
curl -X POST http://localhost:3000/sniffs \
  -H "Content-Type: application/json" \
  -d '{"co2": 9.43, "humidity": 0.4, "temperature": 10.5, "data": "metadata: single_room"}'
```

* Insert data in batch from sensor device into database

```bash
curl -X POST http://localhost:3000/sniffs/batch \
  -H "Content-Type: application/json" \
  -d '[
    {"co2": 4.50, "humidity": 0.4, "temperature": 10.5, "data": "metadata: room_a"},
    {"co2": 5.20, "humidity": 0.5, "temperature": 40.5, "data": "metadata: room_b"},
    {"co2": 4.10, "humidity": 0.2, "temperature": 30.5, "data": "metadata: room_c"}
  ]'
```

### 4.2 How to format JSON for Web-Socket call
> [!NOTE]
> The **payload_id** is returned with the intention of storing the data until a confirmation has been given that the batch has been successfully inserted.
* Get all stored data from sensor device

```json
{"command": "selectAll"}
```

* Insert one data from sensor device into database

```json
{
   "command": "insert",
   "payload_id": "random_uuid",
   "sniff": {"co2": 0.8, "humidity": 0.5, "temperature": 30.8, "data": "metadata: single_room"}
}
```

* Insert data in batch from sensor device into database

```json
{
   "command": "insertMany",
   "payload_id": "random_uuid",
   "sniffs": [
    {"co2": 0.2, "humidity": 0.4, "temperature": 10.5, "data": "metadata: room_a"},
    {"co2": 0.5, "humidity": 0.5, "temperature": 30.8, "data": "metadata: room_b"},
    {"co2": 0.9, "humidity": 0.6, "temperature": 46.3, "data": "metadata: room_c"}
  ]
}
```

