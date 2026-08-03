#include <cassert>
#include <string>

#include "drive_position.h"

int main()
{
	DriveCommand command;
	std::string error;

	assert(ParseDriveCommand("hello", command, error) ==
		DriveCommandResult::NotDriveCommand);
	assert(ParseDriveCommand(
		"!driveposition 123.5 -456.25 13.75", command, error) ==
		DriveCommandResult::Parsed);
	assert(command.kind == DriveCommandKind::Position);
	assert(command.target.x == 123.5f);
	assert(command.target.y == -456.25f);
	assert(command.target.z == 13.75f);

	assert(ParseDriveCommand(
		"!driveto 10 20 30 2500", command, error) ==
		DriveCommandResult::Parsed);
	assert(command.kind == DriveCommandKind::To);
	assert(command.durationMilliseconds == 2500);
	assert(ParseDriveCommand("!drivecancel", command, error) ==
		DriveCommandResult::Parsed);
	assert(command.kind == DriveCommandKind::Cancel);
	assert(ParseDriveCommand("!drivestatus", command, error) ==
		DriveCommandResult::Parsed);
	assert(command.kind == DriveCommandKind::Status);

	assert(ParseDriveCommand("!driveposition", command, error) ==
		DriveCommandResult::Error);
	assert(ParseDriveCommand(
		"!driveposition 1 2 3 trailing", command, error) ==
		DriveCommandResult::Error);
	assert(ParseDriveCommand(
		"!driveposition nan 2 3", command, error) ==
		DriveCommandResult::Error);
	assert(ParseDriveCommand(
		"!driveposition 20001 2 3", command, error) ==
		DriveCommandResult::Error);
	assert(ParseDriveCommand("!driveto 1 2 3 99", command, error) ==
		DriveCommandResult::Error);
	assert(ParseDriveCommand("!driveto 1 2 3 60001", command, error) ==
		DriveCommandResult::Error);
	assert(ParseDriveCommand("!drivecancel now", command, error) ==
		DriveCommandResult::Error);

	DriveMotion motion;
	assert(motion.Start({ 0, 0, 0 }, { 10, 20, 30 }, 1000, 5000, error));
	assert(motion.IsActive());
	auto early = motion.Sample(4999);
	assert(early.active && !early.completed);
	assert(early.position.x == 0);
	auto start = motion.Sample(5000);
	assert(start.active && !start.completed);
	assert(start.position.x == 0);
	auto middle = motion.Sample(5500);
	assert(middle.active && !middle.completed);
	assert(middle.position.x == 5);
	assert(middle.position.y == 10);
	assert(middle.position.z == 15);
	auto finish = motion.Sample(6000);
	assert(!finish.active && finish.completed);
	assert(finish.position.x == 10);
	assert(!motion.IsActive());

	assert(!motion.Start({ 0, 0, 0 }, { 1, 1, 1 }, 99, 0, error));
	assert(motion.Start({ 1, 2, 3 }, { 4, 5, 6 }, 100, 0, error));
	motion.Cancel();
	assert(!motion.IsActive());
	return 0;
}
