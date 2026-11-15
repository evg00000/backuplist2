CREATE TABLE disc (
    id            INTEGER PRIMARY KEY,                 -- number written on the disc
    disc_type     TEXT NOT NULL,                       -- BD-R / DVD / CD
    capacity_gb   INTEGER,                             -- 25 / 50 / 100 / 5 / 9 / 1
    layers        INTEGER,
    written       TEXT,                                -- burn date, YYYY-MM-DD
    content_type  TEXT,                                -- Video / Audio / ... (free text)
    contents      TEXT,                                -- what is on the disc, ';'-separated
    priority      INTEGER CHECK (priority IN (1,2,3)), -- 1 irreplaceable .. 3 replaceable
    status        TEXT CHECK (status IN ('ok','partial','dead')),  -- after the last check
    last_checked  TEXT,
    brand         TEXT,                                -- brand on the box
    media_id      TEXT,                                -- real manufacturer code
    vol_label     TEXT,
    vol_serial    TEXT,                                -- volume serial, identifies the disc
    hash_file     TEXT,                                -- source CSV, if imported
    notes         TEXT
);
CREATE TABLE file (
    id       INTEGER PRIMARY KEY,
    disc_id  INTEGER NOT NULL REFERENCES disc(id) ON DELETE CASCADE,
    path     TEXT NOT NULL,                            -- from the disc root, '/' separated
    sha256   TEXT NOT NULL,
    size     INTEGER
);
CREATE INDEX file_disc ON file(disc_id);
CREATE INDEX file_hash ON file(sha256);
CREATE TABLE check_log (
    id        INTEGER PRIMARY KEY,
    disc_id   INTEGER NOT NULL REFERENCES disc(id) ON DELETE CASCADE,
    checked   TEXT NOT NULL,
    status    TEXT NOT NULL CHECK (status IN ('ok','partial','dead')),
    bad_files INTEGER,
    tool      TEXT,
    notes     TEXT
);
CREATE VIRTUAL TABLE search USING fts5(
    disc_id UNINDEXED, kind UNINDEXED, text,
    tokenize = 'unicode61 remove_diacritics 2'
)
/* search(disc_id,kind,text) */;
CREATE VIEW v_media_stats AS
SELECT media_id, group_concat(DISTINCT brand) AS brands,
       disc_type || ' ' || capacity_gb || 'GB' AS kind, count(*) AS discs,
       sum(status = 'ok') AS ok, sum(status = 'partial') AS partial, sum(status = 'dead') AS dead
FROM disc GROUP BY media_id, kind ORDER BY dead DESC, partial DESC
/* v_media_stats(media_id,brands,kind,discs,ok,partial,dead) */;
CREATE VIEW v_copies AS
SELECT sha256, count(DISTINCT disc_id) AS copies, group_concat(DISTINCT disc_id) AS discs,
       min(path) AS sample_path
FROM file GROUP BY sha256
/* v_copies(sha256,copies,discs,sample_path) */;
CREATE VIEW v_attention AS
SELECT id, priority, disc_type, capacity_gb, layers, status, media_id, contents
FROM disc WHERE status <> 'ok' OR (priority = 1 AND layers = 2)
ORDER BY priority, CASE status WHEN 'dead' THEN 0 WHEN 'partial' THEN 1 ELSE 2 END, id
/* v_attention(id,priority,disc_type,capacity_gb,layers,status,media_id,contents) */;
