CREATE TABLE `Movie` (
	`id` INT NOT NULL,
	`title` VARCHAR(100) NOT NULL,
	`year` INT,
	`rating` VARCHAR(10),
	`company` VARCHAR(50),
	PRIMARY KEY(`id`),
	CHECK(LENGTH(`title`) > 0) -- All movies must have a title that is a non-empty string
) ENGINE = INNODB;

CREATE TABLE `Actor` (
	`id` INT NOT NULL,
	`last` VARCHAR(20),
	`first` VARCHAR(20),
	`sex` VARCHAR(6),
	`dob` DATE,
	`dod` DATE,
	PRIMARY KEY(`id`),
	-- If both date of death and date of birth exist, the date of death must come after the date of birth
	-- An actor must either have a first or a last name that is non null and non-empty
	CHECK((`dod` IS NOT NULL and `dob` IS NOT NULL and `dob` < `dod`) or `dod` IS NULL),
	CHECK((`last` IS NOT NULL and LENGTH(`last`) > 0) or (`first` IS NOT NULL and LENGTH(`first`) > 0))
) ENGINE = INNODB;

CREATE TABLE `Director` (
	`id` INT NOT NULL,
	`last` VARCHAR(20),
	`first` VARCHAR(20),
	`dob` DATE,
	`dod` DATE,
	PRIMARY KEY(`id`),
	-- If both date of death and date of birth exist, the date of death must come after the date of birth
	-- A director must either have a first or a last name that is non null and non-empty
	CHECK((`dod` IS NOT NULL and `dob` IS NOT NULL and `dob` < `dod`) or `dod` IS NULL),
	CHECK((`last` IS NOT NULL and LENGTH(`last`) > 0) or (`first` IS NOT NULL and LENGTH(`first`) > 0))
) ENGINE = INNODB;

CREATE TABLE `MovieGenre` (
	`mid` INT NOT NULL,
	`genre` VARCHAR(20) NOT NULL,
	UNIQUE(`mid`, `genre`),
	FOREIGN KEY(`mid`) references Movie(`id`) -- A MovieGenre must reference a valid Movie entry
) ENGINE = INNODB;

CREATE TABLE `MovieDirector` (
	`mid` INT NOT NULL,
	`did` INT NOT NULL,
	UNIQUE(`mid`, `did`),
	FOREIGN KEY(`mid`) references Movie(`id`),   -- A MovieDirector must reference a valid Movie entry
	FOREIGN KEY(`did`) references Director(`id`) -- A MovieDirector must reference a valid Director entry
) ENGINE = INNODB;

CREATE TABLE `MovieActor` (
	`mid` INT NOT NULL,
	`aid` INT NOT NULL,
	`role` VARCHAR(50) NOT NULL,
	UNIQUE(`mid`, `aid`, `role`),
	FOREIGN KEY(`mid`) references Movie(`id`), -- A MovieActor must reference a valid Movie entry
	FOREIGN KEY(`aid`) references Actor(`id`)  -- A MovieActor must reference a valid Actor entry
) ENGINE = INNODB;

CREATE TABLE `Review` (
	`name` VARCHAR(20) NOT NULL,
	`time` TIMESTAMP NOT NULL,
	`mid` INT NOT NULL,
	`rating` INT NOT NULL,
	`comment` VARCHAR(500),
	UNIQUE(`name`, `mid`),                     -- A single Review entry is allowed for a single Movie and a single given name
	FOREIGN KEY(`mid`) references Movie(`id`), -- A Review must reference a valid Movie entry
	CHECK(`rating` >= 0 and `rating` <= 5)     -- A Review rating must be between 0 and 5, inclusive (i.e. rating out of 5 stars)
) ENGINE = INNODB;

CREATE TABLE `MaxPersonID` (
	`id` INT NOT NULL
) ENGINE = INNODB;

CREATE TABLE `MaxMovieID` (
	`id` INT NOT NULL
) ENGINE = INNODB;