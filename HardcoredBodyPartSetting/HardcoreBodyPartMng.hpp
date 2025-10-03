#pragma once
#include <sstream>
#include "internal/class_vtbls.h"

/*
BGSBodyPartData
*/

class TESForm;
class TESRace;
class BGSBodyPartData;

enum class MatchResult {
	MatchFailed,
	MatchSuccess,
};


struct MultPair {
	enum class Type {
		Crippled, UnCrippled,
	};

	float _crippledMult = -1.0;
	float _unCrippleMult = -1.0;
	MultPair() :_crippledMult(-1.0f) , _unCrippleMult(-1.0f){}
	MultPair(float _crippled, float _unCripped) :_crippledMult(_crippled), _unCrippleMult(_unCripped) {}
	float GetMult(Type _type) const {
		switch (_type)
		{
		case MultPair::Type::Crippled:
			return _crippledMult;
		case MultPair::Type::UnCrippled:
			return _unCrippleMult;
		default:
			return -1.0f;
		}
	}
	void SetMult(Type _type,float _val)  {
		switch (_type)
		{
		case MultPair::Type::Crippled:
			_crippledMult = _val;
			return;
		case MultPair::Type::UnCrippled:
			_unCrippleMult = _val;
			return;
		default:
			return;
		}
	}
	bool Valid() const {
		return (_crippledMult >= 0 && _unCrippleMult >= 0);
	}
};

template<class V>
struct GetResult {
	MatchResult _success{ MatchResult::MatchFailed };
	const V& _ret;

	GetResult(const V& ret) { _ret = ret; }
	bool success() const { return _success == MatchResult::MatchSuccess; }
	const V& result() const { return _ret; }
	void setSuccess(MatchResult _setter) { _success = _setter; }
	void setResult(const V& _setter) { _ret = _setter; }
};

template<class K, class V>
class InsertReadMap {
	std::unordered_map<K, V> _map;
	V _matchFailed;

public:
	InsertReadMap() {}
	using Result = GetResult<V>;

	void __forceinline InsertOrModifiy(K _key, const V& _val) {
		if (const auto& itor = _map.find(_key);itor != _map.end())
		{
			V& _valRef = itor->second;
			_valRef = _val;
		}
		else {
			_map.emplace(_key, _val);
		}
	}

	
	std::enable_if_t< std::is_same_v<V,MultPair>, void> __forceinline
		InsertOrModifiy(K _key, float _val, MultPair::Type _type) {
		if (const auto& itor = _map.find(_key);itor != _map.end())
		{
			MultPair& _valRef = itor->second;
			_valRef.SetMult(_type, _val);
		}
		else {
			MultPair newPair{1.0f, 1.0f};
			newPair.SetMult(_type, _val);
			_map.emplace(_key, newPair);
		}
	}

	Result __forceinline GetValRefConst(K _key) {
		Result _res{ _matchFailed };
		if (const auto& itor = _map.find(_key);itor != _map.end())
		{
			_res.setSuccess(MatchResult::MatchSuccess);
			_res.setResult(itor->second);
		}

		return _res;
	}

	std::enable_if_t< std::is_same_v<V, MultPair>, float> __forceinline
		GetDamageMult(K _key, MultPair::Type _type) const {
		float _res = -1;
		if (const auto& itor = _map.find(_key);itor != _map.end())
		{
			const MultPair& _pair = itor->second;
			_res = _pair.GetMult(_type);
		}

		return _res;
	}

	auto begin() { return _map.begin(); }
	auto end() { return _map.end(); }
	auto cbegin() const { return _map.cbegin();}
	auto cend() const { return _map.cend(); }

	std::enable_if_t< std::is_same_v<V, MultPair>, std::string> __forceinline
		DebugInfo() const {
		
		std::stringstream _ss{};
		for (const auto& _pair : _map)
		{
			_ss << "{ " << _pair.first << " : UnCrippled Mult: " << _pair.second.GetMult(MultPair::Type::UnCrippled) <<
				", Crippled Mult: " << _pair.second.GetMult(MultPair::Type::Crippled) << " }\n";
		}
		std::string _ret = _ss.str();
		if (_ret.empty()) {
			_ret = "Empty Config!";
		}
		return _ret;
	}

	
};


using HitLocConfig = InsertReadMap<SInt32, MultPair>;

template<class K>
class DamageMultConfig {
	using Result = GetResult<MultPair>;
	const float _failedToMatch = -1;
	std::unordered_map<K, HitLocConfig> _objectMap;
public:
	void __forceinline InsertOrModifiy(K _key, SInt32 _loc, float _mult, MultPair::Type _isCrippled) {
		if (const auto& itor = _objectMap.find(_key);itor != _objectMap.end())
		{
			HitLocConfig& _hitlocConfig = itor->second;
			_hitlocConfig.InsertOrModifiy(_loc,_mult, _isCrippled);
		}
		else {
			HitLocConfig _hitlocConfig;
			_hitlocConfig.InsertOrModifiy(_loc, _mult, _isCrippled); 
			_objectMap.emplace(_key,_hitlocConfig); 
		}
	}
	/*
		Get Damage Mult Config
		Return -1 When Not Found
	*/
	float __forceinline GetDamageMult(K _key,SInt32 _loc, MultPair::Type _isCrippled) {
		if (const auto& itor = _objectMap.find(_key);itor != _objectMap.end())
		{
			const HitLocConfig& _hitlocConfig = itor->second;
			return _hitlocConfig.GetDamageMult(_loc,_isCrippled);
		}

		return -1;
	} 
	auto begin() { return _objectMap.begin(); }
	auto end() { return _objectMap.end(); }
	auto cbegin() const { return _objectMap.cbegin(); }
	auto cend() const { return _objectMap.cend(); }

	
};


