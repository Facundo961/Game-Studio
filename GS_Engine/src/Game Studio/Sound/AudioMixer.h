#pragma once

#include "Containers/Id.h"
#include "Containers/HashMap.hpp"

#include "Player.h"
#include "Containers/Pair.h"
#include "Containers/Array.hpp"

class AudioBuffer;

class AudioMixerChannelEffect
{
	/**
	* \brief Defines the effect's name. Used to refer to it.
	*/
	Id effectName;

	/**
	 * \brief Determines the effects intensity when used in a channel.
	 */
	float effectIntensity = 0.0f;
	
public:
	virtual ~AudioMixerChannelEffect();
	
	virtual void Process(const AudioBuffer& _AudioBuffer) = 0;
};

/**
 * \brief Structure to specify the details of the deletion of an audio channel effect.\n
 * Like the fade out time, or the fade out function.
 */
struct AudioMixerChannelEffectRemoveParameters
{
	/**
	 * \brief Determines the time it takes for this effect to be faded out.\n
	 * If KillTime is 0 the effect will be deleted immediately.
	 */
	float FadeOutTime = 0.0f;

	/**
	 * \brief Pointer to the function to be used for fading out the effect. If any fading out is applied at all.
	 */
	void(*FadeFunction)() = nullptr;
};

class AudioMixer
{
	class AudioMixerChannel
	{
		friend class AudioMixer;
		
		/**
		 * \brief Defines the type for a Pair holding a bool to determine whether the sound is virtualized, and a Player* to know which Player to grab the data from.
		 */
		using PlayingSounds = Pair<bool, Player*>;


		/**
		 * \brief Determines how strong this channel sounds.
		 */
		float mixVolume = 0.0f;

		/**
		 * \brief Defines the channel's name. Used to refer to it from the mixer.
		 */
		Id channelName;

		/**
		 * \brief Holds an array of sounds which are to be played.
		 */
		FVector<PlayingSounds> playingSounds;
		
		/**
		 * \brief Holds the collection of effects this channel has. Every channel can have a maximum of 10 simultaneous effects running on it.
		 */
		Array<AudioMixerChannelEffect*, 10> effects;

	public:
		~AudioMixerChannel()
		{
			for(auto& e : effects)
			{
				delete e;
			}
		}
		
		void SetMixVolume(const float _MixVolume) { mixVolume = _MixVolume; }

		/**
		 * \brief Adds and effect to the channel.
		 * \tparam _T Class of effect
		 * \return Effect* to the newly created effect. Could be used to set parameters.
		 */
		template<class _T>
		AudioMixerChannelEffect* AddEffect()
		{
			AudioMixerChannelEffect* new_effect = new _T();
			effects.push_back(new_effect);
			return new_effect;
		}

		void RemoveEffect(const AudioMixerChannelEffectRemoveParameters& _ERP);
	};
	
	/**
	 * \brief Stores every channel available.
	 */
	HashMap<AudioMixerChannel> channels;
	
public:
	void OnUpdate();
	
	void RegisterNewChannel(const AudioMixerChannel& _Channel)
	{
		//channels.TryEmplace;
		channels.Get(0).effects;
	}

	AudioMixerChannel& GetChannel(const Id& _Id)
	{
		return channels.Get(_Id.GetID());
	}
};
