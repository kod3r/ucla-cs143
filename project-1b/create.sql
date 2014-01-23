CREATE TABLE `Movie` (
	`id` INT NOT NULL,
	`title` VARCHAR(100) NOT NULL,
	`year` INT,
	`rating` VARCHAR(10),
	`company` VARCHAR(50)
);

CREATE TABLE `Actor` (
	`id` INT NOT NULL,
	`last` VARCHAR(20),
	`first` VARCHAR(20),
	`sex` VARCHAR(6),
	`dob` DATE,
	`dod` DATE
);

CREATE TABLE `Director` (
	`id` INT NOT NULL,
	`last` VARCHAR(20),
	`first` VARCHAR(20),
	`dob` DATE,
	`dod` DATE	
);

CREATE TABLE `MovieGenre` (
	`mid` INT NOT NULL,
	`genre` VARCHAR(20) NOT NULL
);

CREATE TABLE `MovieDirector` (
	`mid` INT NOT NULL,
	`did` INT NOT NULL
);

CREATE TABLE `MovieActor` (
	`mid` INT NOT NULL,
	`aid` INT NOT NULL,
	`role` VARCHAR(50) NOT NULL
);

CREATE TABLE `Review` (
	`name` VARCHAR(20) NOT NULL,
	`time` TIMESTAMP NOT NULL,
	`mid` INT NOT NULL,
	`rating` INT NOT NULL,
	`comment` VARCHAR(500)
);

CREATE TABLE `MaxPersonID` (
	`id` INT NOT NULL
);

CREATE TABLE `MaxMovieID` (
	`id` INT NOT NULL
);