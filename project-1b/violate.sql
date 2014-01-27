--- Movie ---
-- id must be non null
INSERT INTO Movie(id, title, year, rating, company)
VALUES (NULL, 'Movie', '2014', 'G', 'Movie Company')

-- Primary key must be unique
INSERT INTO Movie(id, title, year, rating, company)
VALUES (1, 'Movie', '2014', 'G', 'Movie Company')

-- Movie titles must be non null
INSERT INTO Movie(id, title, year, rating, company)
VALUES (65536, NULL, '2014', 'G', 'Movie Company')

-- Movie titles must be non-empty strings
INSERT INTO Movie(id, title, year, rating, company)
VALUES (65536, '', '2014', 'G', 'Movie Company')


--- Actor ---
-- id must be non null
INSERT INTO Actor(id, last, first, sex, dob, dod)
VALUES (NULL, 'Smith', 'Joe', 'Male', '1980-1-1', NULL)

-- Primary key id must be unique
INSERT INTO Actor(id, last, first, sex, dob, dod)
VALUES (1, 'Smith', 'Joe', 'Male', '1980-1-1', NULL)

-- If both dob and dod are specified, dob must come before dod
INSERT INTO Actor(id, last, first, sex, dob, dod)
VALUES (65536, 'Smith', 'Joe', 'Male', '1980-1-1', '1970-1-1')

-- Either first or last names must be non null and non-empty
INSERT INTO Actor(id, last, first, sex, dob, dod)
VALUES (65536, '', '', 'Male', '1980-1-1', NULL)

-- Either first or last names must be non null and non-empty
INSERT INTO Actor(id, last, first, sex, dob, dod)
VALUES (65536, NULL, NULL, 'Male', '1980-1-1', NULL)


--- Director ---
-- ids must be non null
INSERT INTO Director(id, last, first, dob, dod)
VALUES (NULL, 'Smith', 'Joe', '1980-1-1', NULL)

-- Primary key id must be unique
INSERT INTO Director(id, last, first, dob, dod)
VALUES (1, 'Smith', 'Joe', '1980-1-1', NULL)

-- If both dob and dod are specified, dob must come before dod
INSERT INTO Director(id, last, first, dob, dod)
VALUES (65536, 'Smith', 'Joe', '1980-1-1', '1970-1-1')

-- Either first or last names must be non null and non-empty
INSERT INTO Director(id, last, first, dob, dod)
VALUES (65536, '', '', '1980-1-1', NULL)

-- Either first or last names must be non null and non-empty
INSERT INTO Director(id, last, first, dob, dod)
VALUES (65536, NULL, NULL, '1980-1-1', NULL)


--- MovieGenre ---
-- mid must be non null
INSERT INTO MovieGenre(mid, genre)
VALUES (NULL, 'Comedy')

-- genre must be non null
INSERT INTO MovieGenre(mid, genre)
VALUES (1, NULL)

-- mid must reference a valid row in the Movie table
-- ERROR 1452 (23000): Cannot add or update a child row: a foreign key constraint fails (`CS143/MovieGenre`, CONSTRAINT `MovieGenre_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO MovieGenre(mid, genre)
VALUES (0, 'Comedy')

-- mid and genre tuples must be unique
INSERT INTO MovieGenre(mid, genre)
VALUES (1, 'Comedy'), (65536, 'Comedy')


--- MovieDirector ---
-- mid must be non null
INSERT INTO MovieDirector(mid, did)
VALUES (NULL, 1)

-- did must be non null
INSERT INTO MovieDirector(mid, did)
VALUES (1, NULL)

-- mid and did tuples must be unique
INSERT INTO MovieDirector(mid, did)
VALUES (1, 1), (1, 1)

-- mid must reference a valid row in the Movie table
-- ERROR 1452 (23000): Cannot add or update a child row: a foreign key constraint fails (`CS143/MovieDirector`, CONSTRAINT `MovieDirector_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO MovieDirector(mid, did)
VALUES (0, 1)

-- did must reference a valid row in the Director table
-- ERROR 1452 (23000): Cannot add or update a child row: a foreign key constraint fails (`CS143/MovieDirector`, CONSTRAINT `MovieDirector_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO MovieDirector(mid, did)
VALUES (1, 0)


--- MovieActor ---
-- mid must be non null
INSERT INTO MovieActor(mid, aid, role)
VALUES (NULL, 1, 'Lead Role')

-- aid must be non null
INSERT INTO MovieActor(mid, aid, role)
VALUES (1, NULL, 'Lead Role')

-- role must be non null
INSERT INTO MovieActor(mid, aid, role)
VALUES (1, 1, NULL)

-- tuples must be unique
INSERT INTO MovieActor(mid, aid, role)
VALUES (1, 1, 'Lead Role'), (1, 1, 'Lead Role')

-- mid must reference a valid row in the Movie table
-- ERROR 1452 (23000): Cannot add or update a child row: a foreign key constraint fails (`CS143/MovieActor`, CONSTRAINT `MovieActor_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO MovieActor(mid, aid, role)
VALUES (0, 1, 'Lead Role')

-- did must reference a valid row in the Actor table
-- ERROR 1452 (23000): Cannot add or update a child row: a foreign key constraint fails (`CS143/MovieActor`, CONSTRAINT `MovieActor_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO MovieActor(mid, aid, role)
VALUES (1, 0, 'Lead Role')


--- Review ---
-- name must be non null
INSERT INTO Review(name, time, mid, rating, comment)
VALUES (NULL, 1390791191, 1, 4, 'Great movie!')

-- time must be non null
INSERT INTO Review(name, time, mid, rating, comment)
VALUES ('Bob Smith', NULL, 1, 4, 'Great movie!')

-- mid must be non null
INSERT INTO Review(name, time, mid, rating, comment)
VALUES ('Bob Smith', 1390791191, NULL, 4, 'Great movie!')

-- rating must be non null
INSERT INTO Review(name, time, mid, rating, comment)
VALUES ('Bob Smith', 1390791191, 1, NULL, 'Great movie!')

-- no two tuples can exist with the same name and same mid
INSERT INTO Review(name, time, mid, rating, comment)
VALUES ('Bob Smith', 1390791191, 1, 4, 'Great movie!'), ('Bob Smith', 1390791191, 1, 1, 'Terrible movie!')

-- mid must reference a valid row in the Movie table
-- ERROR 1452 (23000): Cannot add or update a child row: a foreign key constraint fails (`CS143/Review`, CONSTRAINT `Review_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO Review(name, time, mid, rating, comment)
VALUES ('Bob Smith', 1390791191, 0, 4, 'Great movie!')

-- rating must be between 0 and 5, inclusive
INSERT INTO Review(name, time, mid, rating, comment)
VALUES ('Bob Smith', 1390791191, 1, -1, 'Great movie!')

-- rating must be between 0 and 5, inclusive
INSERT INTO Review(name, time, mid, rating, comment)
VALUES ('Bob Smith', 1390791191, 1, 10, 'Great movie!')


--- MaxPersonID ---
-- id must be non null
INSERT INTO MaxPersonID(id)
VALUES (NULL)


--- MaxMovieID ---
-- id must be non null
INSERT INTO MaxMovieID(id)
VALUES (NULL)
