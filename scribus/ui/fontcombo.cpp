/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          fontcombo.cpp  -  description
                             -------------------
    begin                : Die Jun 17 2003
    copyright            : (C) 2003 by Franz Schmid
    email                : Franz.Schmid@altmuehlnet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QAbstractItemView>
#include <QEvent>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QStringList>

#include "fontcombo.h"
#include "iconmanager.h"
#include "prefsmanager.h"
#include "sccombobox.h"
#include "scpage.h"
#include "scribusdoc.h"
#include "util.h"

FontCombo::FontCombo(QWidget* pa) : QComboBox(pa)
{
	prefsManager = PrefsManager::instance();
	ttfFont = IconManager::instance()->loadPixmap("font_truetype16.png");
	otfFont = IconManager::instance()->loadPixmap("font_otf16.png");
	psFont = IconManager::instance()->loadPixmap("font_type1_16.png");
	substFont = IconManager::instance()->loadPixmap("font_subst16.png");
	setEditable(true);
	QFont f(font());
	f.setPointSize(f.pointSize()-1);
	setFont(f);
	RebuildList(0);
}

void FontCombo::RebuildList(ScribusDoc *currentDoc, bool forAnnotation, bool forSubstitute)
{
	clear();
	QMap<QString, QString> rlist;
	rlist.clear();
	SCFontsIterator it(prefsManager->appPrefs.fontPrefs.AvailFonts);
	for ( ; it.hasNext(); it.next())
	{
		if (it.current().usable())
		{
			if (currentDoc != NULL)
			{
				if (currentDoc->DocName == it.current().localForDocument() || it.current().localForDocument().isEmpty())
					rlist.insert(it.currentKey().toLower(), it.currentKey());
			}
			else
				rlist.insert(it.currentKey().toLower(), it.currentKey());
		}
	}
	for (QMap<QString,QString>::Iterator it2 = rlist.begin(); it2 != rlist.end(); ++it2)
	{
		ScFace fon = prefsManager->appPrefs.fontPrefs.AvailFonts[it2.value()];
		if (! fon.usable() )
			continue;
		ScFace::FontType type = fon.type();
		if ((forAnnotation) && ((type == ScFace::TYPE1) || (type == ScFace::OTF) || fon.subset()))
			continue;
		if (forSubstitute && fon.isReplacement())
			continue;
		if (fon.isReplacement())
			addItem(substFont, it2.value());
		else if (type == ScFace::OTF)
			addItem(otfFont, it2.value());
		else if (type == ScFace::TYPE1)
			addItem(psFont, it2.value());
		else if (type == ScFace::TTF)
			addItem(ttfFont, it2.value());
	}
	QAbstractItemView *tmpView = view();
	int tmpWidth = tmpView->sizeHintForColumn(0);
	if (tmpWidth > 0)
		tmpView->setMinimumWidth(tmpWidth + 24);
}

FontComboH::FontComboH(QWidget* parent, bool labels) :
		QWidget(parent),
		fontFaceLabel(0),
		fontStyleLabel(0),
		showLabels(labels)
{
	currDoc = NULL;
	prefsManager = PrefsManager::instance();
	ttfFont = IconManager::instance()->loadPixmap("font_truetype16.png");
	otfFont = IconManager::instance()->loadPixmap("font_otf16.png");
	psFont = IconManager::instance()->loadPixmap("font_type1_16.png");
	substFont = IconManager::instance()->loadPixmap("font_subst16.png");
	fontComboLayout = new QGridLayout(this);
	fontComboLayout->setMargin(0);
	fontComboLayout->setSpacing(0);
	int col=0;
	if (showLabels)
	{
		fontFaceLabel=new QLabel("", this);
		fontStyleLabel=new QLabel("", this);
		fontComboLayout->addWidget(fontFaceLabel,0,0);
		fontComboLayout->addWidget(fontStyleLabel,1,0);
		fontComboLayout->setColumnStretch(1,10);
		col=1;
	}
	fontFamily = new ScComboBox(this);
	fontFamily->setEditable(true);
	fontComboLayout->addWidget(fontFamily,0,col);
	fontStyle = new ScComboBox(this);
	fontComboLayout->addWidget(fontStyle,1,col);
	isForAnnotation = true;  // this is merely to ensure that the list is rebuilt
	RebuildList(0);
	connect(fontFamily, SIGNAL(activated(int)), this, SLOT(familySelected(int)));
	connect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
	languageChange();
}

void FontComboH::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
	}
	else
		QWidget::changeEvent(e);
}

void FontComboH::languageChange()
{
	if(showLabels)
	{
		fontFaceLabel->setText( tr("Family:"));
		fontStyleLabel->setText( tr("Style:"));
	}
	fontFamily->setToolTip( tr("Font Family of Selected Text or Text Frame"));
	fontStyle->setToolTip( tr("Font Style of Selected Text or Text Frame"));
}

void FontComboH::familySelected(int id)
{
	disconnect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
	QString curr = fontStyle->currentText();
	fontStyle->clear();
	QString fntFamily = fontFamily->itemText(id);
	QStringList slist, styleList = prefsManager->appPrefs.fontPrefs.AvailFonts.fontMap[fntFamily];
	for (QStringList::ConstIterator it = styleList.begin(); it != styleList.end(); ++it)
	{
		SCFonts::ConstIterator fIt = prefsManager->appPrefs.fontPrefs.AvailFonts.find(fntFamily + " " + *it);
		if (fIt != prefsManager->appPrefs.fontPrefs.AvailFonts.end())
		{
			if (!fIt->usable() || fIt->isReplacement())
				continue;
			slist.append(*it);
		}
	}
	slist.sort();
	fontStyle->addItems(slist);
	if (slist.contains(curr))
		setCurrentComboItem(fontStyle, curr);
	else if (slist.contains("Regular"))
		setCurrentComboItem(fontStyle, "Regular");
	else if (slist.contains("Roman"))
		setCurrentComboItem(fontStyle, "Roman");
	emit fontSelected(fontFamily->itemText(id) + " " + fontStyle->currentText());
	connect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
}

void FontComboH::styleSelected(int id)
{
	emit fontSelected(fontFamily->currentText() + " " + fontStyle->itemText(id));
}

QString FontComboH::currentFont()
{
	return fontFamily->currentText() + " " + fontStyle->currentText();
}

void FontComboH::setCurrentFont(QString f)
{
	QString family = prefsManager->appPrefs.fontPrefs.AvailFonts[f].family();
	QString style = prefsManager->appPrefs.fontPrefs.AvailFonts[f].style();
	// If we already have the correct font+style, nothing to do
	if ((fontFamily->currentText() == family) && (fontStyle->currentText() == style))
		return;
	bool familySigBlocked = fontFamily->blockSignals(true);
	bool styleSigBlocked = fontStyle->blockSignals(true);
	setCurrentComboItem(fontFamily, family);
	fontStyle->clear();
	QStringList slist = prefsManager->appPrefs.fontPrefs.AvailFonts.fontMap[family];
	slist.sort();
	QStringList ilist;
	if (currDoc != NULL)
	{
		for (QStringList::ConstIterator it3 = slist.begin(); it3 != slist.end(); ++it3)
		{
			SCFonts::ConstIterator fIt = prefsManager->appPrefs.fontPrefs.AvailFonts.find(family + " " + *it3);
			if (fIt != prefsManager->appPrefs.fontPrefs.AvailFonts.end())
			{
				if (!fIt->usable() || fIt->isReplacement())
					continue;
				if ((currDoc->DocName == fIt->localForDocument()) || (fIt->localForDocument().isEmpty()))
					ilist.append(*it3);
			}
		}
		fontStyle->addItems(ilist);
	}
	else
		fontStyle->addItems(slist);
	setCurrentComboItem(fontStyle, style);
	fontFamily->blockSignals(familySigBlocked);
	fontStyle->blockSignals(styleSigBlocked);
}

void FontComboH::RebuildList(ScribusDoc *currentDoc, bool forAnnotation, bool forSubstitute)
{
	// if we already have the proper fonts loaded, we need to do nothing
	if ((currDoc == currentDoc) && (forAnnotation == isForAnnotation) && (isForSubstitute == forSubstitute))
		return;
	currDoc = currentDoc;
	isForAnnotation = forAnnotation;
	isForSubstitute = forSubstitute;
	bool familySigBlocked = fontFamily->blockSignals(true);
	bool styleSigBlocked = fontStyle->blockSignals(true);
	fontFamily->clear();
	fontStyle->clear();
	QStringList rlist = prefsManager->appPrefs.fontPrefs.AvailFonts.fontMap.keys();
	QMap<QString, ScFace::FontType> flist;
	flist.clear();
	for (QStringList::ConstIterator it2 = rlist.begin(); it2 != rlist.end(); ++it2)
	{
		if (currentDoc != NULL)
		{
			QStringList slist = prefsManager->appPrefs.fontPrefs.AvailFonts.fontMap[*it2];
			slist.sort();
			for (QStringList::ConstIterator it3 = slist.begin(); it3 != slist.end(); ++it3)
			{
				if ( prefsManager->appPrefs.fontPrefs.AvailFonts.contains(*it2 + " " + *it3))
				{
					const ScFace& fon(prefsManager->appPrefs.fontPrefs.AvailFonts[*it2 + " " + *it3]);
					ScFace::FontType type = fon.type();
					if (!fon.usable() || fon.isReplacement() || !(currentDoc->DocName == fon.localForDocument() || fon.localForDocument().isEmpty()))
						continue;
					if ((forAnnotation) && ((type == ScFace::TYPE1) || (type == ScFace::OTF) || (fon.subset())))
						continue;
					if ((forSubstitute) && fon.isReplacement())
						continue;
					flist.insert(*it2, fon.type());
					break;
				}
			}
		}
		else
		{
			QMap<QString, QStringList>::ConstIterator fmIt = prefsManager->appPrefs.fontPrefs.AvailFonts.fontMap.find(*it2);
			if (fmIt == prefsManager->appPrefs.fontPrefs.AvailFonts.fontMap.end())
				continue;
			if (fmIt->count() <= 0)
				continue;
			QString fullFontName = (*it2) + " " + fmIt->at(0);
			ScFace fon = prefsManager->appPrefs.fontPrefs.AvailFonts[fullFontName];
			if ( !fon.usable() || fon.isReplacement() )
				continue;
			ScFace::FontType type = fon.type();
			if ((forAnnotation) && ((type == ScFace::TYPE1) || (type == ScFace::OTF) || (fon.subset())))
				continue;
			if ((forSubstitute) && fon.isReplacement())
				continue;
			flist.insert(*it2, fon.type());
		}
	}
	for (QMap<QString, ScFace::FontType>::Iterator it2a = flist.begin(); it2a != flist.end(); ++it2a)
	{
		ScFace::FontType type = it2a.value();
		// Replacement fonts were systematically discarded in previous code
		/*if (fon.isReplacement())
			fontFamily->addItem(substFont, it2a.value());
		else */if (type == ScFace::OTF)
			fontFamily->addItem(otfFont, it2a.key());
		else if (type == ScFace::TYPE1)
			fontFamily->addItem(psFont, it2a.key());
		else if (type == ScFace::TTF)
			fontFamily->addItem(ttfFont, it2a.key());
	}
	QString family = fontFamily->currentText();
	QStringList slist = prefsManager->appPrefs.fontPrefs.AvailFonts.fontMap[family];
	slist.sort();
	QStringList ilist;
	if (currentDoc != NULL)
	{
		for (QStringList::ConstIterator it = slist.begin(); it != slist.end(); ++it)
		{
			SCFonts::ConstIterator fIt = prefsManager->appPrefs.fontPrefs.AvailFonts.find(family + " " + *it);
			if (fIt != prefsManager->appPrefs.fontPrefs.AvailFonts.end())
			{
				if (!fIt->usable() || fIt->isReplacement())
					continue;
				ilist.append(*it);
			}
		}
		fontStyle->addItems(ilist);
	}
	else
		fontStyle->addItems(slist);
	fontFamily->blockSignals(familySigBlocked);
	fontStyle->blockSignals(styleSigBlocked);
}
