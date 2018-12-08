#include "audioplayer.h"

AudioPlayer::AudioPlayer(AudioPlayerSlider *slider, CrowdedHellGUI *parent) :
	m_fmodNotInit(true), m_pos(0), m_length(0), m_volume(0.6f), m_parent(parent), m_slider(slider), m_fmodSystem(nullptr), m_music(nullptr), m_channel(nullptr)
{
	connect(this, SIGNAL(sendMessage(MessageType, QString, QString)),
			parent, SLOT(sendMessage(MessageType, QString, QString)));

	connect(this, SIGNAL(playedOrPaused(bool)), parent, SLOT(musicPlayedOrPaused(bool)));

	connect(this, SIGNAL(positionChanged(unsigned int)), parent, SLOT(musicPositionChanged(unsigned int)));

	__initializeFmodSystem();

	m_timer = QObject::startTimer(10);
}

void AudioPlayer::timerEvent(QTimerEvent *ev)
{
	if(ev->timerId() == m_timer)
	{
		if(m_channel == nullptr)
		{
			m_pos = 0;
			positionChanged(0);
			return;
		}


		unsigned int last_pos = m_pos;
		m_channel->getPosition(&m_pos, FMOD_TIMEUNIT_MS);
		if(last_pos != m_pos)
		{
			positionChanged(m_pos);
			if(m_pos >= m_length)
				playedOrPaused(false);
		}

		m_fmodSystem->update();
	}
}

void AudioPlayer::reselectMusic(const QString &path)
{
	playOrPause(false);

	if(m_music != nullptr)
		m_music->release();
	m_music = nullptr;

	m_parent->updateMusicLength(0);
	m_pos = 0;
	positionChanged(0);

	FMOD_RESULT result;

	result = m_fmodSystem->createSound(path.toLocal8Bit().data(), FMOD_2D, nullptr, &m_music);
	if(result != FMOD_OK)
	{
		sendMessage(MessageType::Error, "FMOD", tr("Failed to load sound file %1 .").arg(QString("\"") + path + QString("\".")));
		m_music = nullptr;
		return;
	}
	else
		sendMessage(MessageType::Info, "FMOD", tr("Loaded sound file %1 .").arg( QString(" \"") + path + QString("\".")));

	result = m_fmodSystem->playSound(m_music, nullptr, true, &m_channel);
	if(result != FMOD_OK)
	{
		sendMessage(MessageType::Error, "FMOD", tr("Failed to create channel."));
		playedOrPaused(false);
		return;
	}
	else
		sendMessage(MessageType::Info, "FMOD", tr("Create channel successfully."));

	m_music->getLength(&m_length, FMOD_TIMEUNIT_MS);
	m_parent->updateMusicLength(m_length);
}

void AudioPlayer::forward(unsigned int frames)
{
	if(m_pos / 20 == m_length / 20)
		changePosition(m_length);
	else
		changePosition((m_pos / 20+ frames) * 20);
}

void AudioPlayer::back(unsigned int frames)
{
	if(m_pos / 20 == 0)
	{
		changePosition(0);
	}
	else
		changePosition((m_pos / 20 - frames) * 20);
}

unsigned int AudioPlayer::getPosition()
{
	if(m_channel == nullptr)
		return 0;
	else
	{
		unsigned int pos;
		m_channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
		return pos;
	}
}

void AudioPlayer::playOrPause(bool play)
{
	FMOD_RESULT result;

	if(play)
	{
		if(m_music == nullptr)
		{
			sendMessage(MessageType::Warning, "FMOD", tr("No Background music was selected or selected music is invalid."));
			playedOrPaused(false);
			m_parent->musicInvalid();
			return;
		}

		__initializeFmodSystem();

		if(m_channel == nullptr)
		{
			sendMessage(MessageType::Info, "FMOD", tr("No channel was found. Trying to create channel..."));
			result = m_fmodSystem->playSound(m_music, nullptr, true, &m_channel);
			if(result != FMOD_OK)
			{
				sendMessage(MessageType::Error, "FMOD", tr("Failed to create channel."));
				playedOrPaused(false);
				return;
			}
			else
				sendMessage(MessageType::Info, "FMOD", tr("Create channel successfully."));
		}

		bool isPaused;
		m_channel->getPaused(&isPaused);
		if(!isPaused)
			return;

		result = m_channel->setVolume(m_volume);
		if(result != FMOD_OK)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to set volume to %1 percent.").arg(QString::number(double(m_volume * 100))));
			playedOrPaused(false);
			return;
		}
		else
			sendMessage(MessageType::Info, "FMOD", tr("Set volume to %1 percent successful.").arg(QString::number(double(m_volume * 100))));

		if(m_pos >= m_length)
		{
			sendMessage(MessageType::Warning, "FMOD", tr("Music is at the end."));
			m_pos = m_length;
			playedOrPaused(false);
			positionChanged(m_length);
			return;
		}

		result = m_channel->setPosition(unsigned(m_pos), FMOD_TIMEUNIT_MS);
		if(result != FMOD_OK)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to set position to %1 miliseconds while the length of music is %2 miliseconds.").arg(QString::number(m_pos), QString::number(m_length)));
			m_pos = m_length;
			positionChanged(m_length);
			playedOrPaused(false);
			return;
		}
		else
			sendMessage(MessageType::Info, "FMOD", tr("Set position to %1 miliseconds successfully while the length of music is %2 miliseconds.").arg(QString::number(m_pos), QString::number(m_length)));


		result = m_channel->setPaused(false);
		if(result != FMOD_OK)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to play music."));
			m_pos = 0;
			playedOrPaused(false);
			return;
		}
		else
			sendMessage(MessageType::Info, "FMOD",  m_pos == 0 ? tr("Music Started.") : tr("Music Resumed."));

		playedOrPaused(true);
	}
	else
	{
		if(m_music == nullptr)
			return;

		if(m_channel == nullptr)
		{
			sendMessage(MessageType::Error, "FMOD", tr("If you have met this message, it means that my codes have logic error. Please report to me, thanks!"));
			playedOrPaused(false);
			return;
		}

		bool isPaused;
		m_channel->getPaused(&isPaused);
		if(isPaused)
			return;

		result = m_channel->setPaused(true);
		if(result != FMOD_OK)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to pause music."));
			m_pos = 0;
			playedOrPaused(false);
			return;
		}
		else
			sendMessage(MessageType::Info, "FMOD",tr("Music paused."));

		playedOrPaused(false);
	}
}

void AudioPlayer::changePosition(unsigned int pos)
{
	FMOD_RESULT result;

	if(m_music == nullptr)
	{
		sendMessage(MessageType::Warning, "FMOD", tr("No Background music was selected or selected music is invalid."));
		playedOrPaused(false);
		m_parent->musicInvalid();
		return;
	}

	__initializeFmodSystem();

	if(m_channel == nullptr)
	{
		sendMessage(MessageType::Info, "FMOD", tr("No channel was found. Trying to create channel..."));
		result = m_fmodSystem->playSound(m_music, nullptr, true, &m_channel);
		if(result != FMOD_OK)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to create channel."));
			playedOrPaused(false);
			return;
		}
		else
			sendMessage(MessageType::Info, "FMOD", tr("Create channel successfully."));
	}

	if(pos >= m_length)
	{
		sendMessage(MessageType::Warning, "FMOD", tr("Position %1 miliseconds is equal with or larger than music length %2 miliseconds, therefore set the position to the end of music.").arg(QString::number(pos), QString::number(m_length)));
		m_pos = m_length;
		result = m_channel->setPosition(m_length, FMOD_TIMEUNIT_MS);
		if(result != FMOD_OK)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to set position to %1 miliseconds while the length of music is %2 miliseconds.").arg(QString::number(m_pos), QString::number(m_length)));
			m_pos = 0;
			m_channel->setPaused(true);
			playedOrPaused(false);
			return;
		}
		m_channel->setPaused(true);
		playedOrPaused(false);
		positionChanged(m_length);
		return;
	}

	m_pos = pos;
	result = m_channel->setPosition(m_pos, FMOD_TIMEUNIT_MS);
	if(result != FMOD_OK)
	{
		sendMessage(MessageType::Error, "FMOD", tr("Failed to set position to %1 miliseconds while the length of music is %2 miliseconds.").arg(QString::number(m_pos), QString::number(m_length)));
		m_pos = 0;
		m_channel->setPaused(true);
		playedOrPaused(false);
		return;
	}

	sendMessage(MessageType::Info, "FMOD", tr("Set position to %1 miliseconds successfully while the length of music is %2 miliseconds.").arg(QString::number(m_pos), QString::number(m_length)));
	positionChanged(pos);
}

void AudioPlayer::__initializeFmodSystem()
{
	FMOD_RESULT result;

	if(m_fmodSystem == nullptr)
	{
		sendMessage(MessageType::Info, "FMOD", tr("Trying to create FMOD system..."));
		result = FMOD::System_Create(&m_fmodSystem);

		if (result != FMOD_OK)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to create FMOD system."));
			m_fmodSystem = nullptr;
			return;
		}
		else
			sendMessage(MessageType::Info, "FMOD", tr("Created FMOD system successfully."));

		sendMessage(MessageType::Info, "FMOD", tr("Trying to load sound drivers..."));
		int driverCount = 0;
		m_fmodSystem->getNumDrivers(&driverCount);

		if (driverCount == 0)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to load sound drivers."));
			delete m_fmodSystem;
			m_fmodSystem = nullptr;
			return;
		}
		else
			sendMessage(MessageType::Info, "FMOD", tr("%1 sound driver(s) was found.").arg(driverCount));
	}

	if(m_fmodNotInit)
	{
		sendMessage(MessageType::Info, "FMOD", tr("Trying to initialize FMOD system..."));
		result = m_fmodSystem->init(36, FMOD_INIT_NORMAL, nullptr);
		if (result != FMOD_OK)
		{
			sendMessage(MessageType::Error, "FMOD", tr("Failed to initialize FMOD system."));
			m_fmodNotInit = true;
			return;
		}
		else
			sendMessage(MessageType::Info, "FMOD", tr("Initialize FMOD system successfully."));

		m_fmodNotInit = false;
	}
}
