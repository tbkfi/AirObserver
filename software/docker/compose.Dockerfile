FROM denoland/deno:2.9.7 AS builder
WORKDIR /app
COPY ./server/main.ts .
RUN deno compile --allow-net --allow-env --output sniff-service main.ts

FROM debian:bookworm-slim
WORKDIR /app
COPY --from=builder /app/sniff-service /app/sniff-service
EXPOSE 3000
CMD ["/app/sniff-service"]

