#include "stdafx.h"
#include "SettingsBar.hpp"
#include "Label.hpp"
#include "Slider.hpp"
#include "Button.hpp"
#include "GUIRenderer.hpp"

SettingsBar::SettingsBar(Ref<CommonGUIStyle> style) : ScrollBox(style)
{
	m_style = style;
	m_container = new LayoutBox();
	m_container->layoutDirection = LayoutBox::Vertical;
	ScrollBox::SetContent(m_container->MakeShared());
}
SettingsBar::~SettingsBar()
{
	ClearSettings();
}

void SettingsBar::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	rd.area = padding.Apply(rd.area);
	ScrollBox::PreRender(rd, inputElement);

	if (!m_shown)
	{
		GUIElementBase* dummy = nullptr;
		ScrollBox::PreRender(rd, dummy);
	}
	else
	{
		ScrollBox::PreRender(rd, inputElement);
	}
}

void SettingsBar::Render(GUIRenderData rd)
{
	if (m_shown)
	{
		// Background
		rd.guiRenderer->RenderRect(rd.area, Color(0.2f, 0.2f, 0.2f, 0.8f));
		rd.area = padding.Apply(rd.area);
		// Content
		ScrollBox::Render(rd);
	}
}

void SettingsBar::AddSetting(float* target, float min, float max, const String& name)
{
	SettingBarSetting* setting = new SettingBarSetting();
	setting->name = Utility::ConvertToWString(name);
	const wchar_t* nw = *setting->name;
	const char* na = *name;
	setting->floatSetting.target = target;
	setting->floatSetting.min = min;
	setting->floatSetting.max = max;
	float v = (target[0] - min) / (max - min);

	LayoutBox* box = new LayoutBox();
	box->layoutDirection = LayoutBox::Vertical;
	LayoutBox::Slot* slot = m_container->Add(box->MakeShared());
	slot->fillX = true;

	{
		// The label
		Label* label = setting->label = new Label();
		box->Add(label->MakeShared());

		// The slider
		Slider* slider = new Slider(m_style);
		slider->SetValue(v);
		slider->OnSliding.Add(setting, &SettingBarSetting::m_SliderUpdate);
		LayoutBox::Slot* sliderSlot = box->Add(slider->MakeShared());
		sliderSlot->fillX = true;
	}

	m_settings.Add(setting, box->MakeShared());
	setting->m_SliderUpdate(v); // Initial update
}
void SettingsBar::AddSetting(int* target, Vector<String> options, int optionsCount, const String& name)
{
	SettingBarSetting* setting = new SettingBarSetting();
	setting->name = Utility::ConvertToWString(name);
	const wchar_t* nw = *setting->name;
	const char* na = *name;
	setting->textSetting.target = target;
	setting->textSetting.options = new Vector<String>(options);
	setting->textSetting.optionsCount = optionsCount;

	LayoutBox* box = new LayoutBox();
	box->layoutDirection = LayoutBox::Vertical;
	LayoutBox::Slot* slot = m_container->Add(box->MakeShared());
	slot->fillX = true;
	LayoutBox* buttonBox = new LayoutBox();

	buttonBox->layoutDirection = LayoutBox::Horizontal;

	// Create Visuals
	{
		// Name Label
		Label* nameLabel = new Label();
		nameLabel->SetFontSize(25);
		nameLabel->SetText(Utility::WSprintf(L"%ls: ", setting->name));
		box->Add(nameLabel->MakeShared());

		// Prev Button
		Button* prevButton = new Button(m_style);
		prevButton->SetText(L"<");
		prevButton->OnPressed.Add(setting, &SettingBarSetting::m_PrevTextSetting);
		LayoutBox::Slot* prevButtonSlot = buttonBox->Add(prevButton->MakeShared());
		prevButtonSlot->fillX = true;

		// Value label
		Label* label = setting->label = new Label();
		label->SetFontSize(40);
		LayoutBox::Slot* valueLabelSlot = buttonBox->Add(label->MakeShared());
		valueLabelSlot->fillX = false;

		// Next Button
		Button* nextButton = new Button(m_style);
		nextButton->SetText(L">");
		nextButton->OnPressed.Add(setting, &SettingBarSetting::m_NextTextSetting);
		LayoutBox::Slot* nextButtonSlot = buttonBox->Add(nextButton->MakeShared());
		nextButtonSlot->fillX = true;
	}
	LayoutBox::Slot* buttonBoxSlot = box->Add(buttonBox->MakeShared());
	buttonBoxSlot->fillX = true;
	m_settings.Add(setting, box->MakeShared());

	setting->m_UpdateTextSetting(0);
}

void SettingsBar::ClearSettings()
{
	for (auto & s : m_settings)
	{
		delete s.first;
		m_container->Remove(s.second);
	}
	m_settings.clear();
}

void SettingsBar::SetShow(bool shown)
{
	m_shown = shown;
}

void SettingBarSetting::m_SliderUpdate(float val)
{
	floatSetting.target[0] = val * (floatSetting.max - floatSetting.min) + floatSetting.min;
	label->SetText(Utility::WSprintf(L"%ls (%f):", name, floatSetting.target[0]));
}

void SettingBarSetting::m_UpdateTextSetting(int change)
{
	textSetting.target[0] += change;
	textSetting.target[0] %= textSetting.optionsCount;
	if (textSetting.target[0] < 0)
		textSetting.target[0] = textSetting.optionsCount - 1;
	WString display = Utility::ConvertToWString((*textSetting.options)[textSetting.target[0]]);
	label->SetText(Utility::WSprintf(L"%ls", display));
}
void SettingBarSetting::m_PrevTextSetting()
{
	m_UpdateTextSetting(-1);
}
void SettingBarSetting::m_NextTextSetting()
{
	m_UpdateTextSetting(1);
}
