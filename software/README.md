# Backend

## 1. Brief Summary


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

### Run the app using docker compose

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
docker compose -f /docker/docker-compose.yml up --build -d

```

#### Remove docker containers

1. Move to folder software:

```bash
cd AirObserver/software
```

2. docker compile command to remove containers:

```bash
docker compose -f /docker/docker-compose.yml up down
```


