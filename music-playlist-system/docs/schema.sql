-- ER model from the PPT (slide 6). The C backend keeps the same entities in memory
-- (hash tables / linked lists) and persists them to data/musicms.db.
-- This script is the relational equivalent, useful for viva and for the MySQL "future scope".

CREATE TABLE users (
  user_id       INT PRIMARY KEY AUTO_INCREMENT,
  username      VARCHAR(30) UNIQUE NOT NULL,
  email         VARCHAR(100) NOT NULL,
  password_hash TEXT NOT NULL,          -- backend: salt + iterated SHA-256
  is_admin      BOOLEAN DEFAULT FALSE,  -- first registered user
  created_at    DATETIME DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE artists (
  artist_id INT PRIMARY KEY AUTO_INCREMENT,
  name      VARCHAR(80) UNIQUE NOT NULL
);
CREATE TABLE songs (
  song_id    INT PRIMARY KEY AUTO_INCREMENT,
  title      VARCHAR(100) NOT NULL,
  artist_id  INT NOT NULL,
  genre      VARCHAR(40),
  mood       VARCHAR(30),
  file_path  TEXT,                      -- NULL for built-in demo songs
  uploader   INT NULL,
  plays      INT DEFAULT 0,
  FOREIGN KEY (artist_id) REFERENCES artists(artist_id),
  FOREIGN KEY (uploader)  REFERENCES users(user_id)
);
CREATE TABLE playlists (
  playlist_id INT PRIMARY KEY AUTO_INCREMENT,
  user_id     INT NOT NULL,
  name        VARCHAR(80) NOT NULL,
  FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);
CREATE TABLE playlist_songs (            -- M:N between playlist and song
  playlist_id INT NOT NULL,
  song_id     INT NOT NULL,
  position    INT NOT NULL,              -- backend: order in the doubly linked list
  added_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (playlist_id, song_id),
  FOREIGN KEY (playlist_id) REFERENCES playlists(playlist_id) ON DELETE CASCADE,
  FOREIGN KEY (song_id)     REFERENCES songs(song_id) ON DELETE CASCADE
);
