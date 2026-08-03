#include "drive_position.h"

#include <cmath>
#include <sstream>

namespace
{
constexpr float MaximumCoordinateMagnitude = 20000.0f;
constexpr uint32_t MinimumDriveDurationMilliseconds = 100;
constexpr uint32_t MaximumDriveDurationMilliseconds = 60000;

bool IsFiniteAndBounded(const DriveVector &position)
{
	return std::isfinite(position.x) && std::isfinite(position.y) &&
		std::isfinite(position.z) &&
		std::fabs(position.x) <= MaximumCoordinateMagnitude &&
		std::fabs(position.y) <= MaximumCoordinateMagnitude &&
		std::fabs(position.z) <= MaximumCoordinateMagnitude;
}

DriveVector Interpolate(
	const DriveVector &start,
	const DriveVector &target,
	float progress)
{
	return {
		start.x + (target.x - start.x) * progress,
		start.y + (target.y - start.y) * progress,
		start.z + (target.z - start.z) * progress
	};
}
}

DriveCommandResult ParseDriveCommand(
	const std::string &command,
	DriveCommand &parsed,
	std::string &error)
{
	std::istringstream input(command);
	std::string name;
	input >> name;
	if(name != "!driveposition" && name != "!driveto" &&
		name != "!drivecancel" && name != "!drivestatus")
		return DriveCommandResult::NotDriveCommand;

	DriveCommand candidate;
	std::string trailing;
	if(name == "!drivecancel" || name == "!drivestatus")
	{
		if(input >> trailing)
		{
			error = name + " does not accept arguments.";
			return DriveCommandResult::Error;
		}
		candidate.kind = name == "!drivecancel"
			? DriveCommandKind::Cancel
			: DriveCommandKind::Status;
	}
	else
	{
		if(!(input >> candidate.target.x >> candidate.target.y >>
			candidate.target.z))
		{
			error = name == "!driveto"
				? "Usage: !driveto <x> <y> <z> <duration-ms>"
				: "Usage: !driveposition <x> <y> <z>";
			return DriveCommandResult::Error;
		}

		if(name == "!driveto")
		{
			unsigned long duration = 0;
			if(!(input >> duration) || duration < MinimumDriveDurationMilliseconds ||
				duration > MaximumDriveDurationMilliseconds)
			{
				error = "Drive duration must be between 100 and 60000 milliseconds.";
				return DriveCommandResult::Error;
			}
			candidate.kind = DriveCommandKind::To;
			candidate.durationMilliseconds = static_cast<uint32_t>(duration);
		}
		else
			candidate.kind = DriveCommandKind::Position;

		if(input >> trailing)
		{
			error = name + " received unexpected trailing input.";
			return DriveCommandResult::Error;
		}
		if(!IsFiniteAndBounded(candidate.target))
		{
			error = "Drive coordinates must be finite and within +/-20000.";
			return DriveCommandResult::Error;
		}
	}

	parsed = candidate;
	error.clear();
	return DriveCommandResult::Parsed;
}

bool DriveMotion::Start(
	const DriveVector &start,
	const DriveVector &target,
	uint32_t durationMilliseconds,
	uint64_t nowMilliseconds,
	std::string &error)
{
	if(!IsFiniteAndBounded(start) || !IsFiniteAndBounded(target))
	{
		error = "Drive motion endpoints must be finite and within +/-20000.";
		return false;
	}
	if(durationMilliseconds < MinimumDriveDurationMilliseconds ||
		durationMilliseconds > MaximumDriveDurationMilliseconds ||
		nowMilliseconds > UINT64_MAX - durationMilliseconds)
	{
		error = "Drive motion duration is outside the supported bounds.";
		return false;
	}

	start_ = start;
	target_ = target;
	startedAtMilliseconds_ = nowMilliseconds;
	finishesAtMilliseconds_ = nowMilliseconds + durationMilliseconds;
	active_ = true;
	error.clear();
	return true;
}

DriveMotionSample DriveMotion::Sample(uint64_t nowMilliseconds)
{
	if(!active_)
		return { target_, false, false };
	if(nowMilliseconds <= startedAtMilliseconds_)
		return { start_, true, false };

	const bool completed = nowMilliseconds >= finishesAtMilliseconds_;
	const float progress = completed
		? 1.0f
		: static_cast<float>(nowMilliseconds - startedAtMilliseconds_) /
			static_cast<float>(finishesAtMilliseconds_ - startedAtMilliseconds_);
	const DriveVector position = Interpolate(start_, target_, progress);
	if(completed)
		active_ = false;
	return { position, !completed, completed };
}

void DriveMotion::Cancel()
{
	active_ = false;
}
