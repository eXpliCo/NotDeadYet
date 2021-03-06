#pragma once

#include "FileReader.h"
#include <map>
#include <Vector.h>
#include <vector>

enum ANIMATION_INFORMATION
{
	BONE_L_WEAPON,
	BONE_R_WEAPON,
	BONE_ROOT,
	BONE_CAMERA_OFFSET,
	MISC_CAMERA_OFFSET_VALUE,

};

struct AnimationQueue
{
	std::vector<std::string> zAnimations;
	std::vector<float> zAnimationTimes;
	bool empty()
	{
		return zAnimations.size() == 0 || zAnimationTimes.size() == 0;
	}
};

static const std::string IDLE_O1				= "idle_01";
static const std::string IDLE_O2				= "idle_02";
static const std::string IDLE_O3				= "idle_03";
static const std::string IDLE_O4				= "idle_04";
static const std::string IDLE_O5				= "idle_05";
static const std::string IDLE_O6				= "idle_06";

static const std::string SPRINT					= "sprint";

static const std::string WALK_FORWARD			= "walk_fwd";
static const std::string WALK_BACKWARD			= "walk_bwd";
static const std::string WALK_RIGHT				= "walk_rwd";
static const std::string WALK_LEFT				= "walk_lwd";

static const std::string JOG_FORWARD			= "jog_fwd";
static const std::string JOG_BACKWARD			= "jog_bwd";
static const std::string JOG_RIGHT				= "jog_rwd";
static const std::string JOG_LEFT				= "jog_lwd";

static const std::string BOW_EQUIP				= "arch_equip";
static const std::string BOW_EQUIPPED_IDLE		= "arch_equiped_idle";
static const std::string BOW_UNEQUIP			= "arch_unequip";
static const std::string BOW_DRAW				= "arch_draw_bow";
static const std::string BOW_DRAW_IDLE			= "arch_draw_bow_idle";
static const std::string BOW_SHOOT_DRAW_ARROW	= "arch_shot_draw_arrow";
static const std::string BOW_UNDRAW				= "arch_undraw_bow";

static const std::string MACHETE_EQUIP			= "mach_equip";
static const std::string MACHETE_UNEQUIP_01		= "mach_unequip_01";
static const std::string MACHETE_UNEQUIP_02		= "mach_unequip_02";
static const std::string MACHETE_ATTACK_01		= "mach_attack_01";
static const std::string MACHETE_ATTACK_02		= "mach_attack_02";
static const std::string MACHETE_IDLE_01		= "mach_idle_01";
static const std::string MACHETE_IDLE_02		= "mach_idle_02";

static const std::string PKNIFE_EQUIP			= "pknife_equip";
static const std::string PKNIFE_ATTACK			= "pknife_attack"; 
static const std::string PKNIFE_IDLE			= "pknife_idle";
static const std::string PKNIFE_UNEQUIP			= "pknife_unequip";

static const std::string FILL_CANTEEN			= "fill_canteen";
static const std::string GENERIC_USE			= "generic_use";
static const std::string GENERIC_CRAFT			= "generic_craft";

static const std::string ANIMAL_ATTACK_01		= "attack_01";
static const std::string ANIMAL_ATTACK_02		= "attack_02";
static const std::string BUSY					= "idle_03";
static const std::string DEAD					= "final_rest";
static const std::string DEATH					= "death";

static const std::string BOW_MODEL				= "BOW";
static const std::string PKNIFE_MODEL			= "PKNIFE";
static const std::string MACHETE_MODEL			= "MACHETE";
static const std::string ARROW_MODEL			= "ARROW";
static const std::string CANTEEN_MODEL			= "CANTEEN";

class AnimationFileReader : public FileReader
{
public:
	AnimationFileReader() {this->zFileName = "";}
	AnimationFileReader(const std::string& filename);
	virtual ~AnimationFileReader();

	bool ReadFromFile();

	unsigned int InterpCommand(const std::string& key);
	const std::string& GetAnimation(const std::string& animationName) const;
	const std::string& GetBindingBone(const unsigned int bone) const;
	const float GetAnimationTime(const std::string& animationName);
	const std::string& GetSubMeshName(const std::string& name);

private:
	std::string zFileName;

	//Maps animation type with animation Name
	std::map<std::string, std::string> zAnimationNames;
	std::map<std::string, float> zAnimationTimes;
	std::map<unsigned int, std::string> zBindingBones;
	std::map<unsigned int, Vector3> zMiscStuff;
	std::map<std::string, std::string> zSubMeshes;
};