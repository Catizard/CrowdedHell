#include "crowdedhellgui.h"
#include "ui_crowdedhellgui.h"

CrowdedHellGUI::CrowdedHellGUI(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::CrowdedHellGUI)
{
	ui->setupUi(this);

	m_languageActions.insert(Language::EN, ui->actionEnglish);
	m_languageActions.insert(Language::ZH_CN, ui->actionSimplifiedChinese);
	m_languageActions.insert(Language::ZH_TW, ui->actionTraditionalChinese);
	m_languageActions.insert(Language::JP, ui->actionJapanese);

	m_translators.insert(Language::ZH_CN, new QTranslator(this));
	m_translators[Language::ZH_CN]->load(":/translations/Trans_zh_cn.qm");
	m_translators.insert(Language::ZH_TW, new QTranslator(this));
	m_translators[Language::ZH_TW]->load(":/translations/Trans_zh_tw.qm");
	m_translators.insert(Language::JP, new QTranslator(this));
	m_translators[Language::JP]->load(":/translations/Trans_jp.qm");
}

CrowdedHellGUI::~CrowdedHellGUI()
{
	delete ui;
}

void CrowdedHellGUI::changeEvent(QEvent *event)
{
	if(event -> type() == QEvent::LanguageChange)
	{

	}
}

void CrowdedHellGUI::on_actionSimplifiedChinese_triggered()
{
	__refreshTranslation(Language::ZH_CN);
}

void CrowdedHellGUI::on_actionEnglish_triggered()
{
	__refreshTranslation(Language::EN);
}

void CrowdedHellGUI::on_actionTraditionalChinese_triggered()
{
	__refreshTranslation(Language::ZH_TW);
}

void CrowdedHellGUI::on_actionJapanese_triggered()
{
	__refreshTranslation(Language::JP);
}

void CrowdedHellGUI::__refreshTranslation(CrowdedHellGUI::Language language)
{
	if(language == Language::EN)
		foreach(auto i, m_translators)
			qApp->removeTranslator(i);
	else
		qApp->installTranslator(m_translators[language]);

	for(auto itr = m_languageActions.begin(); itr != m_languageActions.end(); itr++)
		if(itr.key() == language)
			itr.value()->setChecked(true);
		else
			itr.value()->setChecked(false);

	ui->retranslateUi(this);
}