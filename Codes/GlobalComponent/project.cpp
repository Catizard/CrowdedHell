#include "project.h"

Project *project()
{
    static Project *uniqueProject = new Project;
    return uniqueProject;
}

Project::~Project()
{

}

Project::Project(QObject *parent) :
    QObject(parent),
    m_changed(false)
{

}

void Project::initialze()
{
    connect(this, &Project::message, logger(), &Logger::message, Qt::UniqueConnection);
    m_initilized = true;
}

bool Project::isValid() const
{
    return !m_projectName.isEmpty() && m_initilized;
};

void Project::newProject()
{
    CreateProjectWizard *wizard = new CreateProjectWizard;
    if(wizard->exec() != QWizard::Accepted)
    {
        delete wizard;
        return;
    }

    QString projectPath = wizard->getPath() + QString("/") + wizard->getProjectName();

    QString musicPath = wizard->getMusicPath();
    if(!QFile(musicPath).exists())
    {
        emit message(Logger::Type::Error, "Project", tr("Selected music file doesn't exist, at : \"%1\".").arg(musicPath));
        delete wizard;
        return;
    }

    if(!QDir().exists(projectPath))
    {
        if(!QDir().mkpath(projectPath))
        {
            emit message(Logger::Type::Error, "Project", tr("Make selected directory failed, at : \"%1\".").arg(projectPath));
            delete wizard;
            return;
        }
    }
    else
    {
        emit message(Logger::Type::Error, "Project", tr("Project is already exists, at : \"%1\".").arg(projectPath));
        delete wizard;
        return;
    }

    if(!QDir().mkpath(projectPath + QString("/Sprites")))
    {
        emit message(Logger::Type::Error, "Project", tr("Make selected directory failed, at : \"%1\".").arg(projectPath) + QString("/sprites"));
        delete wizard;
        return;
    }
    if(!QDir().mkpath(projectPath + QString("/Sounds")))
    {
        emit message(Logger::Type::Error, "Project", tr("Make selected directory failed, at : \"%1\".").arg(projectPath) + QString("/sounds"));
        delete wizard;
        return;
    }
    if(!QDir().mkpath(projectPath + QString("/Backgrounds")))
    {
        emit message(Logger::Type::Error, "Project", tr("Make selected directory failed, at : \"%1\".").arg(projectPath) + QString("/backgrounds"));
        delete wizard;
        return;
    }
    if(!QDir().mkpath(projectPath + QString("/Paths")))
    {
        emit message(Logger::Type::Error, "Project", tr("Make selected directory failed, at : \"%1\".").arg(projectPath) + QString("/paths"));
        delete wizard;
        return;
    }

    m_projectPath = projectPath;
    m_projectName = wizard->getProjectName();
    QFile projectFile(m_projectPath + QString("/") + m_projectName + QString(".chproj"));
    if(!projectFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        emit message(Logger::Type::Error, "Project", tr("Create project file failed, at : \"%1\".").arg(m_projectPath + QString("/") + m_projectName + QString(".chproj")));
        m_projectPath.clear();
        m_projectName.clear();
        delete wizard;
        return;
    }

    QTextStream textStream(&projectFile);
    textStream << "This is just a project file for Crowded Hell." << endl;
    textStream << wizard->getAuthor() << endl;
    textStream << wizard->getDate().toString() << endl;
    textStream << QFileInfo(musicPath).fileName() << endl;

    projectFile.flush();
    projectFile.close();

    if(!QFile::copy(musicPath, projectPath + QString("/") + QFileInfo(musicPath).fileName()))
    {
        emit message(Logger::Type::Error, "Project", tr("Cannot copy music file from \"%1\" to \"%2\".").arg(musicPath).arg(projectPath + QString("/") + QFileInfo(musicPath).fileName()));
        m_projectPath.clear();
        m_projectName.clear();
        delete wizard;
        return;
    }

    m_author = wizard->getAuthor();
    m_createDate = wizard->getDate();
    m_musicFile = QFileInfo(musicPath).fileName();

    emit message(Logger::Type::Info, "Project", tr("Project \"%1\" created.").arg(m_projectName));
    QApplication::processEvents();
    emit projectOpened(m_projectPath + QString("/") + m_projectName + QString(".chproj"));
    QApplication::processEvents();
    emit musicSelected(projectPath + QString("/") + QFileInfo(musicPath).fileName());

    delete wizard;
};

void Project::openProject()
{
    QString projectFilePath = QFileDialog::getOpenFileName(nullptr, tr("Select a project"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "Crowded Hell Project(*.chproj)");
    if(projectFilePath.isEmpty())
        return;

    openProject(projectFilePath);
}

void Project::openProject(const QString &projectFilePath)
{
    QFile projectFile(projectFilePath);
    if(!projectFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        emit message(Logger::Type::Error, "Projcet Manager", tr("Open selected project file failed, at : \"%1\".").arg(projectFilePath));
        return;
    }

    QTextStream textStream(&projectFile);
    QStringList datas = textStream.readAll().split("\n");
    projectFile.close();

    if(datas.size() < 4)
    {
        emit message(Logger::Type::Error, "Project", tr("Project file format abnormal."));
        return;
    }

    QString projectPath = QFileInfo(projectFilePath).absolutePath();

    closeProject();

    m_projectName = QFileInfo(projectFilePath).fileName().remove(QString(".chproj"));
    m_projectPath = QFileInfo(projectFilePath).absolutePath();
    m_author = datas[1];
    m_createDate = QDateTime::fromString(datas[2]);
    m_musicFile = datas[3];

    if(!QFile(projectPath + QString("/") + m_musicFile).exists())
    {
        emit message(Logger::Type::Error, "Project", tr("Cannot find the music file of project, at : \"%1\".").arg(m_projectPath + QString("/") + m_musicFile));
        return;
    }

    emit message(Logger::Type::Info, "Project", tr("Project \"%1\" opened.").arg(m_projectName));
    QApplication::processEvents();
    emit projectOpened(m_projectPath + QString("/") + m_projectName + QString(".chproj"));
    QApplication::processEvents();
    emit musicSelected(projectPath + QString("/") + m_musicFile);
}

void Project::saveChange()
{

}

void Project::saveTo(const QString &path)
{

};

void Project::closeProject()
{
    if(!isValid())
        return;

    if(m_changed)
    {
        int shouldSave = QMessageBox::No;
        if(!options()->autoSave())
            shouldSave = QMessageBox::question(nullptr, tr("Confirm"), tr("Would you like to save your project before close it?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

        if(shouldSave == QMessageBox::Cancel)
            return;

        if(options()->autoSave() || shouldSave == QMessageBox::Yes)
            saveChange();
    }

    emit projectClosed();
    QApplication::processEvents(); 
    emit message(Logger::Type::Info, "Project", tr("Project \"%1\" closed.").arg(m_projectName));

    m_projectName.clear();
    m_projectPath.clear();
    m_temporaryPath.clear();
    m_author.clear();
    m_musicFile.clear();

    m_changed = false;

}

void Project::reselectMusic()
{
    QString musicFilePath = QFileDialog::getOpenFileName(nullptr, tr("Select Music File"), qApp->applicationDirPath(), tr("Music File(*.mp3 *.wav)"));
    reselectMusic(musicFilePath);
}

void Project::reselectMusic(const QString &musicPath)
{
    if(!isValid())
    {
        emit message(Logger::Type::Error, "Project", tr("No project is open. Please create or open a project before."));
        return;
    }

    if(!QFile(musicPath).exists())
    {
        emit message(Logger::Type::Error, "Project", tr("Selected music file doesn't exist, at : \"%1\".").arg(musicPath));
        return;
    }

    if(!QFile::copy(musicPath, m_projectPath + QString("/") + QFileInfo(musicPath).fileName()))
    {
        emit message(Logger::Type::Error, "Project", tr("Cannot copy music file from \"%1\" to \"%2\".").arg(musicPath).arg(m_projectPath + QString("/") + QFileInfo(musicPath).fileName()));
        return;
    }

    QFile projectFile(m_projectPath + QString("/") + m_projectName + QString(".chproj"));
    if(!projectFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        emit message(Logger::Type::Error, "Project", tr("Project file doesn't exist, at : \"%1\".").arg(m_projectPath + QString("/") + m_projectName + QString(".chproj")));
        return;
    }

    QTextStream textStream(&projectFile);
    QStringList datas = textStream.readAll().split("\n");
    projectFile.close();

    if(datas.size() < 4)
    {
        emit message(Logger::Type::Error, "Project", tr("Project file format is abnormal."));
        return;
    }

    if(QFile(m_projectPath + "/" + datas[3]).exists())
        if(!QFile(m_projectPath + "/" + datas[3]).remove())
            emit message(Logger::Type::Warning, "Project", tr("Failed remove music file \"%1\".").arg(m_projectPath + "/" + datas[3]));

    datas[3] = QFileInfo(musicPath).fileName();

    if(!projectFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        emit message(Logger::Type::Error, "Project", tr("Project file doesn't exist, at : \"%1\".").arg(m_projectPath + QString("/") + m_projectName + QString(".chproj")));
        return;
    }

    foreach(auto i, datas)
        textStream << i << endl;
    projectFile.flush();

    QApplication::processEvents();
    emit musicSelected(m_projectPath + QString("/") + QFileInfo(musicPath).fileName());

    return;
};

const QString &Project::temporaryPath() const
{
    return m_temporaryPath;
}

const QString &Project::musicFile() const
{
    return m_musicFile;
}

const QDateTime &Project::createDate() const
{
    return m_createDate;
}

const QString &Project::author() const
{
    return m_author;
}

const QString &Project::projectPath() const
{
    return m_projectPath;
}

const QString &Project::projectName() const
{
    return m_projectName;
};
