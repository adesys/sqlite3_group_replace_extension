
Group replace function for sqlite3.
====================================
This aggregate function works just like group_concat, but replaces key with value in a string.
This can be used to make parameterized text with multiple parameters (key) which will be
replaced with the corresponding value using this aggregation function.

Usage:
-----------------
group_replace(string, key, value)
example:

    SELECT group_replace('just a simple test', 'test', 'succesful test');

Compile the code:
-----------------
    gcc -g -fPIC -shared ./sqlite3_group_replace_extension.c  -o sqlite3_group_replace_extension.so

Load the extension:
-----------------
    select load_extension('./sqlite3_group_replace_extension.so');

Example:
-----------------
    -- in this example we create a table wich contains parameterized texts
    -- a text can contain as much parameters as you want,
    CREATE TABLE examples   (
      "example_id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
      "text"       TEXT
    );

    -- parameters are stored in a key/value table.
    -- for each parameter in the text (table examples) we create a key/value row
    CREATE TABLE key_values (
     "key_value_id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
     "key"          VARCHAR,
     "value"        VARCHAR,
     "example_id"   INTEGER,
     CONSTRAINT "fk_example_id"
        FOREIGN KEY("example_id")
        REFERENCES "examples"("example_id")
        ON DELETE CASCADE
        ON UPDATE CASCADE
    );

    CREATE INDEX "key_values.example_id_idx" ON "key_values"("example_id");

    -- lets insert some data
    -- first example has two parameters
    INSERT INTO "examples" ("text") VALUES ("Hello, my name is NAME, I'am AGE years old");
    INSERT INTO key_values ("example_id", "key", "value") VALUES (1,'NAME', 'Anthony');
    INSERT INTO key_values ("example_id", "key", "value") VALUES (1,'AGE', '25');

    -- you can use any name for the parameter, just use this name as key in the key/value table
    INSERT INTO "examples" ("text") VALUES ("I am actualy %dog# %*$$ years old");
    INSERT INTO key_values ("example_id", "key", "value") VALUES (2,'%dog# %*$$', '38');

    -- this example demonstrates multiple pararameters with the same name (key)
    -- all these parameters will be replaced by the value
    INSERT INTO "examples" ("text") VALUES ("Please open the door, knock, knock, knock,...!");
    INSERT INTO key_values ("example_id", "key", "value") VALUES (3,'knock,', 'penny');

    -- load the extension, this way sqlite knows the new `group_replace` function
    SELECT load_extension('./sqlite3_group_replace_extension.so');

    -- execute the query below
    SELECT group_replace(e.text, kv.key, kv. value) FROM examples e
    JOIN key_values kv on kv.example_id=e.example_id
    GROUP BY e.example_id;

    -- this should result in:
    -- Hello, my name is Anthony, I'am 25 years old
    -- I am actualy 38 years old
    -- Please open the door, penny penny penny...!


Performance:
-----------------
I tested above example on 100.000 texts, and the query was executed on my machine in 0.23 seconds.
(tested using sqlite timer)

    .timer on

The same data using group_concat was done in 0.21 seconds, a little bit faster as expected, but not bad.
(replacing texts is more expensive than simple concatination)

    SELECT group_concat(kv.key, kv. value) FROM examples e
    JOIN key_values kv on kv.example_id=e.example_id
    GROUP BY e.example_id;


Licence / inprovements
-----------------
Copyright 2016 Adesys b.v.
This software is licensed under LGPL.
(use it "as is", and send improvements by making a pull request.)

