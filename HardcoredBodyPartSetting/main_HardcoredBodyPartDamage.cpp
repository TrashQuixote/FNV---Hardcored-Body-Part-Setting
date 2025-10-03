#pragma once
#include "HardcoreBodyPartMng.hpp"
#include "Gathering_Utility.h"
#include "Gathering_Node.h"
#include "RoughINIReader.h"
//#include "backward.hpp"
//#include "utilities/IConsole.h"
//NoGore is unsupported in xNVSE

IDebugLog		gLog("HardcoredBodyPartMult.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;
NVSEMessagingInterface* g_messagingInterface{};
NVSEInterface* g_nvseInterface{};
NVSEEventManagerInterface* g_eventInterface{};
_InventoryRefGetForID InventoryRefGetForID;
_InventoryRefCreate InventoryRefCreate;

//static void PrintCallStack() {
//	std::stringstream _ss;
//	using namespace backward;
//	StackTrace st;
//	st.load_here(32);
//	Printer pt;
//	pt.print(st, _ss);
//	__WriteLog2(true, "%s",_ss.str().c_str());
//}

template <class T>
inline T& singleton() {
	static T instance;
	return instance;
}

constexpr bool DebugLogging = false;

enum HardcoredPsychosisConfig {
	Disable = 0,
	NPC_Only = 1,
	PC_And_NPC = 2,
};

static UINT8 HardcoredPsychosis = 0u;
static float HardcoredPsychosisMult = 1.0f 
;
using BodyPartDmgConfig = DamageMultConfig<const BGSBodyPartData*>;
using CreatureTypeDmgConfig = DamageMultConfig<UINT8>;

class DamageConfigMng {

	DamageConfigMng(DamageConfigMng&&) = delete;

	BodyPartDmgConfig _bodyPartDmgConfig;
	CreatureTypeDmgConfig _creatureTypeDmgConfig;

	bool __forceinline _type_check(const TESForm* _form) {
		return IS_TYPE(_form, BGSBodyPartData);
	}
	std::string CreatureTypeDebugInfo() {
		std::stringstream _ss;
		for (const auto& _pair : _creatureTypeDmgConfig) {
			_ss << "Creature Type: " << (UINT32)_pair.first << ": \n";
			_ss << _pair.second.DebugInfo();
		}
		std::string _ret = _ss.str();
		if (_ret.empty())
		{
			_ret = "Creature Type Config Is Empty!";
		}
		return _ret;
	}

	std::string BPDataDebugInfo() {
		std::stringstream _ss;
		for (const auto& _pair : _bodyPartDmgConfig) {
			_ss << "Body Part Data: " << _pair.first->GetEditorID() << ": \n";
			_ss << _pair.second.DebugInfo();
		}
		std::string _ret = _ss.str();
		if (_ret.empty())
		{
			_ret = "Body Part Data Config Is Empty!";
		}
		return _ret;
	}

public:
	DamageConfigMng() = default;
	enum class ConfigType {
		BodyPart, CreatureType,
	};
	const BodyPartDmgConfig& GetBPDmgConfig() const {
		return _bodyPartDmgConfig;
	}

	void __forceinline InsertOrModifiy(TESForm* raceOrBody, SInt32 _loc, float _mult, MultPair::Type _isCrippled) {
		if (!_type_check(raceOrBody)) [[unlikely]] {
			return;
		}

		_bodyPartDmgConfig.InsertOrModifiy((BGSBodyPartData*)(raceOrBody), _loc, _mult, _isCrippled);
	}

	void __forceinline InsertOrModifiy(UINT8 _type, SInt32 _loc, float _mult, MultPair::Type _isCrippled) {

		_creatureTypeDmgConfig.InsertOrModifiy(_type, _loc, _mult, _isCrippled);
	}

	float __forceinline GetDamageMult(const BGSBodyPartData* _obj, SInt32 _loc, MultPair::Type _isCrippled) {
		if (!_type_check(_obj)) [[unlikely]] {
			return -1.0f;
		}
		return _bodyPartDmgConfig.GetDamageMult(_obj, _loc, _isCrippled);

	}

	float __forceinline GetDamageMult(UINT8 _type, SInt32 _loc, MultPair::Type _isCrippled) {
		return _creatureTypeDmgConfig.GetDamageMult(_type, _loc, _isCrippled);

	}

	std::string DebugInfo(ConfigType _type) {
		switch (_type)
		{
		case DamageConfigMng::ConfigType::BodyPart:
			return BPDataDebugInfo();
		case DamageConfigMng::ConfigType::CreatureType:
			return CreatureTypeDebugInfo();
		default:
			return "Get DebugInfo Failed.";
		}
	}
};

static CreatureType GetCreatureTypeByStr(const std::string& _creatureTypeName) {
	std::string _temp = _creatureTypeName;
	for (char& _ch : _temp){
		_ch = std::tolower(_ch);
	}

	if (_temp == "animal") return CreatureType::Animal;
	if (_temp == "mutatedanimal") return CreatureType::Mutated_Animal;
	if (_temp == "mutatedinsect") return CreatureType::Mutated_Insect;
	if (_temp == "abomination") return CreatureType::Abomination;
	if (_temp == "supermutant") return CreatureType::Supermutant;
	if (_temp == "feralghoul") return CreatureType::Feral_Ghoul;
	if (_temp == "robot") return CreatureType::Robot;
	if (_temp == "giant") return CreatureType::Giant;
	if (_temp == "notcreature") return CreatureType::NotCreature;

	return CreatureType::Count;
	
}


bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	_MESSAGE("query");

	// fill out the info structure
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "ArmorDamageReformula";
	info->version = 114;

	// version checks
	if (nvse->nvseVersion < PACKED_NVSE_VERSION)
	{
		_ERROR("NVSE version too old (got %08X expected at least %08X)", nvse->nvseVersion, PACKED_NVSE_VERSION);
		return false;
	}

	if (!nvse->isEditor)
	{
		if (nvse->runtimeVersion < RUNTIME_VERSION_1_4_0_525)
		{
			_ERROR("incorrect runtime version (got %08X need at least %08X)", nvse->runtimeVersion, RUNTIME_VERSION_1_4_0_525);
			return false;
		}

		if (nvse->isNogore)
		{
			_ERROR("NoGore is not supported");
			return false;
		}
	}
	else
	{
		if (nvse->editorVersion < CS_VERSION_1_4_0_518)
		{
			_ERROR("incorrect editor version (got %08X need at least %08X)", nvse->editorVersion, CS_VERSION_1_4_0_518);
			return false;
		}
	}

	// version checks pass
	// any version compatibility checks should be done here
	return true;
}



namespace HardcoredBodyPartMult {

	namespace fs = std::filesystem;

	static bool ReadGeneralConfig(const std::string& file_name, MultPair::Type _type) {
		__WriteLog2(true, ">> Reading Config");
		fs::path config_root_path = fs::current_path();
		config_root_path += R"(\Data\NVSE\Plugins\HardcoredBodyPartConfig\GeneralConfig\)";
		if (!fs::exists(config_root_path)) {
			gLog.Message("ReadGenericConfig path not exist");
			return false;
		}

		__WriteLog2(true, "Reading Config From %s", file_name.c_str());
		roughinireader::INIReader _ini{ config_root_path };

		//auto ret = _ini.SetCurrentINIFileName("ArmorDamageFormulaConfig.ini");
		auto ret = _ini.SetCurrentINIFileName(file_name);
		if (!ret.has_value()) {
			gLog.FormattedMessage("Failed to set generic config filename : %s", ret.error().message());
			return false;
		}
		ret = _ini.ConstructSectionMap();
		if (!ret.has_value()) {
			gLog.FormattedMessage("Failed to construct section map : %s", ret.error().message());
			return false;
		}

		auto* _section_map = _ini.GetSectionMapCst();

		if (_section_map)
		{
			for (const auto& _pair : *_section_map)
			{
				const std::string& _sectionName = _pair.first;

				CreatureType _creatureType = GetCreatureTypeByStr(_sectionName);

				if (_creatureType == CreatureType::Count)
				{
					__WriteLog2(true, "Invalid Section Name: %s", _sectionName);
					continue;
				}

				const auto* _settings = _pair.second.get();
				if (_settings)
				{
					__WriteLog2(true, "Section (%s) Has %u Settings", _sectionName.c_str(), _settings->size());
					for (const auto& _pair : *_settings)
					{
						const std::string& _s_hLoc = _pair.first;
						const std::string& _s_mult = _pair.second;
					
						SInt32 _hLoc = std::stoi(_s_hLoc);
						float _fMult = std::stof(_s_mult);

						singleton<DamageConfigMng>().InsertOrModifiy(_creatureType, _hLoc,_fMult, _type);
					}
				}
				else {
					__WriteLog2(true, "Get Settings Map From Section %s Failed", _sectionName.c_str());
				}
			}
		}
		else {
			__WriteLog2(true,"Section Map Not Exist");
		}
		

	}


	static bool ReadSingleConfig(const char* _relative_path, const std::string& file_name, MultPair::Type _type) {
		gLog.Message("ReadSingleConfig");
		fs::path config_root_path = fs::current_path();
		config_root_path.append( _relative_path);
		if (!fs::exists(config_root_path)) {
			__WriteLog2(true, "Path not exist: %s", config_root_path.string());
			return false;
		}
		__WriteLog2(true,"Reading Overwrite Config For Body Part Dmg, File Name: %s", file_name.c_str());
		roughinireader::INIReader _ini{ config_root_path };

		//auto ret = _ini.SetCurrentINIFileName("ArmorDamageFormulaConfig.ini");
		auto ret = _ini.SetCurrentINIFileName(file_name);
		if (!ret.has_value()) {
			gLog.FormattedMessage("Failed to set generic config filename : %s", ret.error().message());
			return false;
		}
		ret = _ini.ConstructSectionMap();
		if (!ret.has_value()) {
			gLog.FormattedMessage("Failed to construct section map : %s", ret.error().message());
			return false;
		}

		auto* _section_map = _ini.GetSectionMapCst();

		if (_section_map)
		{
			for (const auto& _pair : *_section_map)
			{
				const std::string& _sectionName = _pair.first;

				TESForm* _bodyPartData =LookupByEditorID(_sectionName.c_str());
				if (!_bodyPartData)
				{
					__WriteLog2(true, "Can't Find Form By EditorID: %s", _sectionName.c_str());
					continue;
				}

				if (!IS_TYPE(_bodyPartData,BGSBodyPartData))
				{
					__WriteLog2(true, "Form: %s is not BGSBodyPartData", _sectionName.c_str());
					continue;
				}

				const auto* _settings = _pair.second.get();
				if (_settings)
				{
					__WriteLog2(true, "Section %s Has %u Settings", _sectionName.c_str(), _settings->size());
					for (const auto& _pair : *_settings)
					{
						const std::string& _s_hLoc = _pair.first;


						const std::string& _s_mult = _pair.second;

						SInt32 _hLoc = std::stoi(_s_hLoc);
						float _fMult = std::stof(_s_mult);

						singleton<DamageConfigMng>().InsertOrModifiy(_bodyPartData, _hLoc, _fMult, _type);
					}
				}
				else {
					__WriteLog2(true, "Get Settings Map From Section %s Failed", _sectionName.c_str());
				}
			}
		}
		else {
			__WriteLog2(true, "Section Map Not Exist");
		}

	}

	static bool ReadBodyPartDataConfig(const char* _relative_path,MultPair::Type _type) {

		fs::path readingDirectory = std::filesystem::current_path().append(_relative_path);
		__WriteLog2(true, "Read Body Part Config Directory: %s", readingDirectory.string());
		if (fs::exists(readingDirectory) && fs::is_directory(readingDirectory)) {
			for (const auto& _entry : fs::directory_iterator(readingDirectory))
			{
				if (_entry.path().extension() != ".ini")
					continue;

				
				if (const auto& _pStr = _entry.path().filename().string();!_pStr.empty())
				{
					__WriteLog2(true, "Reading From Files: %s", _pStr.c_str());
					ReadSingleConfig(_relative_path, _pStr, _type);
				}
			}
		}
		else {

		}
	}

// Fxxk u MSVC
// Fxxk u Microsoft
#undef max
#undef min

	static bool __forceinline ValueInRange(float _val, float _rVal_1, float _rVal_2) {
		float _max = std::max(_rVal_1, _rVal_2);
		float _min = std::min(_rVal_1, _rVal_2);


		return _val >= _min && _val <= _max;
	}

	

	static bool __forceinline CheckIsUpperTorso(NiNode* _rootNode,const char* _topNode, const char* _bottomNode,const NiVector3& _hitPos) {
		if (_rootNode)
		{
			if (NiAVObject* _nNeck = _rootNode->GetBlock(_topNode)){
				if (NiAVObject* _nSpine = _rootNode->GetBlock(_bottomNode)){
					const NiVector3& _NeckPos = _nNeck->WorldTranslate();
					const NiVector3& _SpinePos = _nSpine->WorldTranslate();

					/*__WriteLog2(DebugLogging, "_hitPos: %.2f, %.2f, %.2f", _hitPos.x, _hitPos.y, _hitPos.z);
					__WriteLog2(DebugLogging, "_NeckPos: %.2f, %.2f, %.2f", _NeckPos.x, _NeckPos.y, _NeckPos.z);
					__WriteLog2(DebugLogging, "_SpinePos: %.2f, %.2f, %.2f", _SpinePos.x, _SpinePos.y, _SpinePos.z);
					__WriteLog2(DebugLogging, "_LowTorsoPos: %.2f, %.2f, %.2f", _btm.x, _btm.y, _btm.z);*/
					
					float _hitToNeck = CalcPosSquareDisSIMD(_hitPos, _NeckPos);
					float _hitToSpine = CalcPosSquareDisSIMD(_hitPos, _SpinePos);
					__WriteLog2(DebugLogging, "Hit To Neck %.2f", _hitToNeck);
					__WriteLog2(DebugLogging, "Hit To Spine %.2f", _hitToSpine);
					return _hitToNeck < _hitToSpine;
				}
				else {
					__WriteLog2(DebugLogging, "Actor Has No Spine Node");
				}
			}
			else {
				__WriteLog2(DebugLogging, "Actor Has No Neck Node");
			}
		}
		else {
			__WriteLog2(DebugLogging, "Actor Has No RootNode");
		}
	}

	static bool __forceinline IsUpperTorso(const Actor* _actor,const NiVector3& _hitPos) {
		auto* _pc = PlayerCharacter::GetSingleton();
		if (_actor == _pc) {
			if (! _pc->IsThirdPerson()){	// 1st
				return CheckIsUpperTorso(_pc->node1stPerson, "Bip01 Neck", "Bip01 Spine",_hitPos);
			}
			else {
				return CheckIsUpperTorso(_actor->GetRefNiNode(), "Bip01 Neck", "Bip01 Spine", _hitPos);
			}
		}
		else {
			return CheckIsUpperTorso(_actor->GetRefNiNode(), "Bip01 Neck", "Bip01 Spine", _hitPos);
		}
	}

	// Check hit location <= 14 before call this function
	static auto __forceinline GetHitLimbIsCrippled(const Actor* _actor, const BGSBodyPartData* _bpData,SInt32 _hLoc) {
		struct Result {
			bool _success = false;
			bool _isCrippled = false;

			bool IsSuccess() const { return _success; }
			bool IsCrippled() const { return _isCrippled; }
		}_ret;
		
		if (_bpData)
		{
			if (BGSBodyPart* _bdPart = _bpData->bodyParts[_hLoc]) {
				ActorValueCode _code = static_cast<ActorValueCode>(_bdPart->actorValue);
				__WriteLog2(DebugLogging, "AV Code: %u", _code);
				float _av = _actor->avOwner.GetActorValue(_code);
				__WriteLog2(DebugLogging, "AV : %.2f", _av);
				_ret._success = true;
				_ret._isCrippled = (_av <= 0.0f);
			}
		}
		else {
			__WriteLog2(DebugLogging,"BGSBodyPartData Is Null");
		}
		return _ret;
	}

	static float __forceinline GetTorsoDamageMult(const Actor* _target, const ActorHitData* _hitData) {
		float _fMult = 1.0f;
		bool _isPC = _target == PlayerCharacter::GetSingleton();

		switch (HardcoredPsychosis)
		{
		case HardcoredPsychosisConfig::Disable:
			return _fMult;
		case HardcoredPsychosisConfig::NPC_Only:
			if (!_isPC && IsUpperTorso(_target, _hitData->impactPos))
			{
				return HardcoredPsychosisMult;
			}
			break;
		case HardcoredPsychosisConfig::PC_And_NPC:
			if (IsUpperTorso(_target, _hitData->impactPos))
			{
				return HardcoredPsychosisMult;
			}
			break;
		default:
			return _fMult;
		}
		return _fMult;
	}

	/*
	Flech Damage Mult From Configs Maps
	_fTemp will be negative when config not exist.
	Check Overwrite Config first, then check General( creature type ) Config.
	When currently hit location not be config for damage mult, will return 1.0f
	*/
	static float __forceinline GetHitDamageMult(const Actor* _target, const ActorHitData* _hitData,const BGSBodyPartData* _hpData, SInt32 _hitLoc, MultPair::Type _type) {
		float _fMult = 1.0f;
		float _fTemp = singleton<DamageConfigMng>().GetDamageMult(_hpData, _hitData->hitLocation, _type);
		CreatureType _cType = GetCreatureType(_target);
		__WriteLog2(DebugLogging, "Damage Mult From BodyPartDataConfig Is %.2f", _fTemp);
		if (_fTemp < 0.0f)
		{
			_fTemp = singleton<DamageConfigMng>().GetDamageMult(_cType, _hitData->hitLocation, _type);
			__WriteLog2(DebugLogging, "Damage Mult From CreatureType Is %.2f", _fTemp);
			if (_fTemp >= 0.0f)
			{
				_fMult = _fTemp;
			}
		}
		else {
			_fMult = _fTemp;
		}

		if (HardcoredPsychosis != HardcoredPsychosisConfig::Disable && _cType == CreatureType::NotCreature) {
			
			if (_hitData->hitLocation == 0)
			{
				_fMult *= GetTorsoDamageMult(_target, _hitData);
				__WriteLog2(DebugLogging, "Hit Upper Torso");
			}
			else {
				__WriteLog2(DebugLogging, "Hit Non-Upper Torso");
			}
			__WriteLog2(DebugLogging, "Final Damage Mult Is %.2f", _fMult);
		}
		return _fMult;
	}
	/*
	The "Edge Case" Is The HitDamage Will Cause Cripple Limb.
	We Will Apply Crippled Damage Mult Only When The Mult > 1.0
	Otherwise We Will Apply UnCrippled Damage Mult.
	*/
	static float __forceinline GetHitDamageMult(const Actor* _target,const ActorHitData* _hitData,SInt32 _hitLoc) {
		const BGSBodyPartData* _hpData = GetBodyPartData(_target);
		float _fMult = 1.0f;

		bool _crippledHit = _hitData->isFlagOn(static_cast<ActorHitData::EnumHitFlags>(ActorHitData::kFlag_CrippleLimb | ActorHitData::kFlag_ExplodeLimb | ActorHitData::kFlag_DismemberLimb ));
		
		if (_crippledHit) [[unlikely]]
		{
			__WriteLog2(DebugLogging,"Edge Cases");
			_fMult = GetHitDamageMult(_target, _hitData, _hpData, _hitLoc,MultPair::Type::Crippled);
			if (_fMult < 1.0f) {
				_fMult = GetHitDamageMult(_target, _hitData, _hpData, _hitLoc, MultPair::Type::UnCrippled);
			}

			return _fMult;
		}

		__WriteLog2(DebugLogging, "Limb Damage: %.2f,DmgMult: %.2f", _hitData->limbDmg, _hitData->dmgMult);
		auto _checkCrippled = GetHitLimbIsCrippled(_target, _hpData, _hitData->hitLocation);

		if (_checkCrippled.IsSuccess()) {
			MultPair::Type _isCrippled = _checkCrippled.IsCrippled() ?
				MultPair::Type::Crippled : MultPair::Type::UnCrippled;

			__WriteLog2(DebugLogging, "Crippled ?: %u", (_isCrippled == MultPair::Type::Crippled ? 1 : 0));

			_fMult = GetHitDamageMult(_target, _hitData, _hpData, _hitLoc, _isCrippled);
		}
		
		return _fMult;
	}

	static CallDetour _actorHitMatter;
	static void __fastcall Actor_OnHitMatter(Actor* _target, void* _edx, ActorHitData* _hitData, bool _unkFlag) {
		if (!_target || !IS_ACTOR(_target)) [[unlikely]] {
			ThisStdCall(_actorHitMatter.GetOverwrittenAddr(), _target, _hitData, _unkFlag);
			return;
		}

		if (!_hitData || !_hitData->source || _hitData->hitLocation < 0 || _hitData->hitLocation > 14
			|| _hitData->healthDmg <= 0.0f) [[unlikely]] {
			ThisStdCall(_actorHitMatter.GetOverwrittenAddr(), _target, _hitData, _unkFlag);
			return;
		}
		

		float _fMult = GetHitDamageMult(_target, _hitData, _hitData->hitLocation);
		_hitData->healthDmg *= _fMult;
		ThisStdCall(_actorHitMatter.GetOverwrittenAddr(), _target, _hitData, _unkFlag);
	}

	static std::string BPDataDebugInfo() {
		const auto& _bpdConfig = singleton<DamageConfigMng>().GetBPDmgConfig();
		
	}

	static bool ReadGeneralConfig() {
		fs::path config_root_path = fs::current_path();
		config_root_path += R"(\Data\NVSE\Plugins\HardcoredBodyPartConfig\)";
		if (!fs::exists(config_root_path)) {
			gLog.Message("ReadGenericConfig path not exist");
			return false;
		}

		roughinireader::INIReader _ini{ config_root_path };

		auto ret = _ini.SetCurrentINIFileName("HardcoredBodyPartConfig.ini");
		if (!ret.has_value()) {
			gLog.FormattedMessage("Failed to set generic config filename : %s", ret.error().message());
			return false;
		}
		ret = _ini.ConstructSectionMap();
		if (!ret.has_value()) {
			gLog.FormattedMessage("Failed to construct section map : %s", ret.error().message());
			return false;
		}

		std::string raw_type_val = "";

		raw_type_val = _ini.GetRawTypeVal("General", "HardcoredPsychosis");
		HardcoredPsychosis = (raw_type_val.empty() ? HardcoredPsychosisConfig::Disable : static_cast<UINT32>(std::stoi(raw_type_val)));

		if (HardcoredPsychosis > HardcoredPsychosisConfig::PC_And_NPC)
		{
			HardcoredPsychosis = HardcoredPsychosisConfig::PC_And_NPC;
		}

		raw_type_val = _ini.GetRawTypeVal("General", "HardcoredPsychosisDamageMult");
		HardcoredPsychosisMult = (raw_type_val.empty() ? 1.0f : std::stof(raw_type_val));

		__WriteLog2(true, "HardcoredPsychosis: %d", HardcoredPsychosis);
		__WriteLog2(true, "HardcoredPsychosisDamageMult: %.2f", HardcoredPsychosisMult);
	}

	static void ConfigReading() {
		ReadGeneralConfig(); 
		ReadGeneralConfig("CrippledConfig.ini", MultPair::Type::Crippled);
		ReadGeneralConfig("UncrippledConfig.ini", MultPair::Type::UnCrippled);
		ReadBodyPartDataConfig(R"(Data\NVSE\Plugins\HardcoredBodyPartConfig\OverwriteConfig\CrippedDamageConfig\)"
			,MultPair::Type::Crippled);
		ReadBodyPartDataConfig(R"(Data\NVSE\Plugins\HardcoredBodyPartConfig\OverwriteConfig\UncrippledDamageConfig\)"
			, MultPair::Type::UnCrippled);
		__WriteLog2(true, singleton<DamageConfigMng>().DebugInfo(DamageConfigMng::ConfigType::CreatureType).c_str());
		__WriteLog2(true, singleton<DamageConfigMng>().DebugInfo(DamageConfigMng::ConfigType::BodyPart).c_str());

	}

	static void InstallHook() {
		//_actorHitData_SetDMG.WriteRelCall(0x9B5702, (UINT32)ActorHitData_SetDmgHook);
		//_testHook.WriteRelCall(0x89A7BD, (UINT32)TestHook);
		//singleton<BPDataPtrConfig>();
		_actorHitMatter.WriteRelCall(0x89A738, (UINT32)Actor_OnHitMatter);

	}
};


// This is a message handler for nvse events
// With this, plugins can listen to messages such as whenever the game loads
void MessageHandler(NVSEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case NVSEMessagingInterface::kMessage_DeferredInit:
		initSingleton();
		HardcoredBodyPartMult::ConfigReading();
		HardcoredBodyPartMult::InstallHook();
		break;
	default:
		break;
	}
}

bool NVSEPlugin_Load(NVSEInterface* nvse)
{
	_MESSAGE("MissileHook load");
	g_pluginHandle = nvse->GetPluginHandle();
	// save the NVSE interface in case we need it later
	g_nvseInterface = nvse;
	NVSEDataInterface* nvseData = (NVSEDataInterface*)nvse->QueryInterface(kInterface_Data);
	InventoryRefGetForID = (_InventoryRefGetForID)nvseData->GetFunc(NVSEDataInterface::kNVSEData_InventoryReferenceGetForRefID);
	InventoryRefCreate = (_InventoryRefCreate)nvseData->GetFunc(NVSEDataInterface::kNVSEData_InventoryReferenceCreateEntry);

	// register to receive messages from NVSE

	if (!nvse->isEditor)
	{
		g_messagingInterface = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
		g_messagingInterface->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);
	}
	return true;
}