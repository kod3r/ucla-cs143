CREATE TABLE `Movie` (
	`id` INT NOT NULL,
	`title` VARCHAR(100) NOT NULL,
	`year` INT,
	`rating` VARCHAR(10),
	`company` VARCHAR(50),
	PRIMARY KEY(`id`)
) ENGINE = INNODB;

CREATE TABLE `Actor` (
	`id` INT NOT NULL,
	`last` VARCHAR(20),
	`first` VARCHAR(20),
	`sex` VARCHAR(6),
	`dob` DATE,
	`dod` DATE,
	PRIMARY KEY(`id`)
) ENGINE = INNODB;

CREATE TABLE `Director` (
	`id` INT NOT NULL,
	`last` VARCHAR(20),
	`first` VARCHAR(20),
	`dob` DATE,
	`dod` DATE,
	PRIMARY KEY(`id`)
) ENGINE = INNODB;

CREATE TABLE `MovieGenre` (
	`mid` INT NOT NULL,
	`genre` VARCHAR(20) NOT NULL,
	UNIQUE(`mid`, `genre`),
	FOREIGN KEY(`mid`) references Movie(`id`)
) ENGINE = INNODB;

CREATE TABLE `MovieDirector` (
	`mid` INT NOT NULL,
	`did` INT NOT NULL,
	UNIQUE(`mid`, `did`),
	FOREIGN KEY(`mid`) references Movie(`id`),
	FOREIGN KEY(`did`) references Director(`id`)
) ENGINE = INNODB;

CREATE TABLE `MovieActor` (
	`mid` INT NOT NULL,
	`aid` INT NOT NULL,
	`role` VARCHAR(50) NOT NULL,
	UNIQUE(`mid`, `aid`, `role`),
	FOREIGN KEY(`mid`) references Movie(`id`),
	FOREIGN KEY(`aid`) references Actor(`id`)
) ENGINE = INNODB;

CREATE TABLE `Review` (
	`name` VARCHAR(20) NOT NULL,
	`time` TIMESTAMP NOT NULL,
	`mid` INT NOT NULL,
	`rating` INT NOT NULL,
	`comment` VARCHAR(500),
	UNIQUE(`name`, `mid`),
	FOREIGN KEY(`mid`) references Movie(`id`)
) ENGINE = INNODB;

CREATE TABLE `MaxPersonID` (
	`id` INT NOT NULL
) ENGINE = INNODB;

CREATE TABLE `MaxMovieID` (
	`id` INT NOT NULL
) ENGINE = INNODB;