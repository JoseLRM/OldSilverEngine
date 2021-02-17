#include "SilverEngine/core.h"

#include "SilverEngine/scene.h"

namespace sv {

	void SpriteComponent::serialize(ArchiveO& archive)
	{
		save_asset(archive, texture);
		archive << texcoord << color << layer;
	}

	void SpriteComponent::deserialize(ArchiveI& archive)
	{
		load_asset(archive, texture);
		archive >> texcoord >> color >> layer;
	}

	void NameComponent::serialize(ArchiveO& archive)
	{
		archive << name;
	}

	void NameComponent::deserialize(ArchiveI& archive)
	{
		archive >> name;
	}

	void CameraComponent::serialize(ArchiveO& archive)
	{
		archive << projection_type << near << far << width << height;
	}

	void CameraComponent::deserialize(ArchiveI& archive)
	{
		archive >> projection_type >> near >> far >> width >> height;
	}

}