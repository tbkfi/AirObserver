CREATE USER airsniffer WITH PASSWORD 'sniff3r3r';
CREATE DATABASE airsniffs;
GRANT ALL PRIVILEGES ON DATABASE airsniffs TO airsniffer;

\c airsniffs

CREATE TABLE sniffs
(
    sniff_id SERIAL NOT NULL,
    co2 INT NOT NULL,
    carb INT NOT NULL,
    data VARCHAR(500) NOT NULL,
    created_at DATE DEFAULT NOW(),
    PRIMARY KEY (sniff_id)
);

GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO airsniffer;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO airsniffer;
