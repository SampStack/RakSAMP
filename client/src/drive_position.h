#pragma once

#include <cstdint>
#include <string>

struct DriveVector
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

enum class DriveCommandKind
{
	Position,
	To,
	Cancel,
	Status
};

struct DriveCommand
{
	DriveCommandKind kind = DriveCommandKind::Status;
	DriveVector target;
	uint32_t durationMilliseconds = 0;
};

enum class DriveCommandResult
{
	NotDriveCommand,
	Parsed,
	Error
};

DriveCommandResult ParseDriveCommand(
	const std::string &command,
	DriveCommand &parsed,
	std::string &error);

struct DriveMotionSample
{
	DriveVector position;
	bool active = false;
	bool completed = false;
};

class DriveMotion
{
public:
	bool Start(
		const DriveVector &start,
		const DriveVector &target,
		uint32_t durationMilliseconds,
		uint64_t nowMilliseconds,
		std::string &error);
	DriveMotionSample Sample(uint64_t nowMilliseconds);
	void Cancel();
	bool IsActive() const { return active_; }
	const DriveVector &Target() const { return target_; }
	uint64_t FinishesAtMilliseconds() const { return finishesAtMilliseconds_; }

private:
	DriveVector start_;
	DriveVector target_;
	uint64_t startedAtMilliseconds_ = 0;
	uint64_t finishesAtMilliseconds_ = 0;
	bool active_ = false;
};
