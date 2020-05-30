#pragma once

//-----------------------------------------------------------------------------
#include "Mesh.h"

//-----------------------------------------------------------------------------
// flagi u�ywane przy odtwarzaniu animacji
enum PLAY_FLAGS
{
	// odtwarzaj raz, w przeciwnym razie jest zap�tlone
	// po odtworzeniu animacji na grupie wywo�ywana jest funkcja Deactive(), je�li jest to grupa
	// podrz�dna to nadrz�dna jest odgrywana
	PLAY_ONCE = 0x01,
	// odtwarzaj od ty�u
	PLAY_BACK = 0x02,
	// wy��cza blending dla tej animacji
	PLAY_NO_BLEND = 0x04,
	// ignoruje wywo�anie Play() je�eli jest ju� ta animacja
	PLAY_IGNORE = 0x08,
	PLAY_STOP_AT_END = 0x10,
	// priorytet animacji
	PLAY_PRIO0 = 0,
	PLAY_PRIO1 = 0x20,
	PLAY_PRIO2 = 0x40,
	PLAY_PRIO3 = 0x60,
	// odtwarza animacj� gdy sko�czy si� blending
	PLAY_BLEND_WAIT = 0x100
};

//-----------------------------------------------------------------------------
// obiekt wykorzystuj�cy Mesh
// zwalnia tekstury override przy niszczeniu
//-----------------------------------------------------------------------------
struct MeshInstance
{
	enum FLAGS
	{
		FLAG_PLAYING = 1 << 0,
		FLAG_ONCE = 1 << 1,
		FLAG_BACK = 1 << 2,
		FLAG_GROUP_ACTIVE = 1 << 3,
		FLAG_BLENDING = 1 << 4,
		FLAG_STOP_AT_END = 1 << 5,
		FLAG_BLEND_WAIT = 1 << 6,
		FLAG_UPDATED = 1 << 7
		// je�li b�dzie wi�cej flagi potrzeba zmian w Read/Write
	};

	struct Group
	{
		Group() : anim(nullptr), state(0), speed(1.f), prio(0), blend_max(0.33f), frame_end(false)
		{
		}

		float time, speed, blend_time, blend_max;
		int state, prio, used_group;
		Mesh::Animation* anim;
		bool frame_end;

		int GetFrameIndex(bool& hit) const { return anim->GetFrameIndex(time, hit); }
		float GetBlendT() const;
		float GetProgress() const { return time / anim->length; }
		bool IsActive() const { return IsSet(state, FLAG_GROUP_ACTIVE); }
		bool IsBlending() const { return IsSet(state, FLAG_BLENDING); }
		bool IsPlaying() const { return IsSet(state, FLAG_PLAYING); }
		void SetProgress(float progress) { time = progress * anim->length; }
	};
	typedef vector<byte>::const_iterator BoneIter;

	explicit MeshInstance(Mesh* mesh, bool preload = false);
	// kontynuuj odtwarzanie animacji
	void Play(int group = 0)
	{
		SetBit(groups[group].state, FLAG_PLAYING);
	}
	// odtwarzaj animacj�
	void Play(Mesh::Animation* anim, int flags, int group);
	// odtwarzaj animacj� o podanej nazwie
	void Play(cstring name, int flags, int group = 0)
	{
		Mesh::Animation* anim = mesh->GetAnimation(name);
		assert(anim);
		Play(anim, flags, group);
	}
	// zatrzymaj animacj�
	void Stop(int group = 0)
	{
		ClearBit(groups[group].state, FLAG_PLAYING);
	}
	// deazktyw�j grup�
	void Deactivate(int group = 0, bool in_update = false);
	// aktualizacja animacji
	void Update(float dt);
	// ustawianie blendingu
	void SetupBlending(int grupa, bool first = true, bool in_update = false);
	// ustawianie ko�ci
	void SetupBones();
	float GetProgress(int group = 0) const
	{
		assert(InRange(group, 0, mesh->head.n_groups - 1));
		return groups[group].GetProgress();
	}
	void SetProgress(float progress, int group = 0)
	{
		assert(InRange(group, 0, mesh->head.n_groups - 1));
		groups[group].SetProgress(progress);
	}
	void DisableAnimations();
	void SetToEnd(cstring anim)
	{
		Mesh::Animation* a = mesh->GetAnimation(anim);
		SetToEnd(a);
	}
	void SetToEnd(Mesh::Animation* anim);
	void SetToEnd();
	void ResetAnimation();
	void Save(FileWriter& f);
	void Load(FileReader& f, int version);
	void Write(StreamWriter& f) const;
	bool Read(StreamReader& f);
	bool ApplyPreload(Mesh* mesh);
	int GetHighestPriority(uint& group);
	int GetUsableGroup(uint group);
	void ClearEndResult();
	bool IsEnded(uint g = 0) const
	{
		return groups[g].frame_end;
	}
	bool IsBlending() const;

	Mesh* mesh;
	float base_speed;
	bool need_update, preload;
	vector<Matrix> mat_bones;
	vector<Mesh::KeyframeBone> blendb;
	vector<Group> groups;
	Matrix* mat_scale;
	void* ptr;
	static void(*Predraw)(void*, Matrix*, int);
};
