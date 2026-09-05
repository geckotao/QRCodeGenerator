#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/filedlg.h>
#include <qrencode.h>

#define U8(s) wxString::FromUTF8(s)

class QRFrame : public wxFrame {
public:
    QRFrame() : wxFrame(nullptr, wxID_ANY, U8("二维码生成器 by geckotao"),
                        wxDefaultPosition, wxSize(600, 520),
                        wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX)) {
        SetMinSize(wxSize(600, 520));

        wxIcon frameIcon;
        if (frameIcon.LoadFile("IDI_ICON1", wxBITMAP_TYPE_ICO_RESOURCE)) {
            SetIcon(frameIcon);
        }
        

        mainPanel = new wxPanel(this);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // 1. 顶部控制区
        wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
        topSizer->Add(new wxStaticText(mainPanel, wxID_ANY, U8("引擎:")),
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        engineCombo = new wxComboBox(mainPanel, wxID_ANY, U8("libqrencode"),
                                     wxDefaultPosition, wxSize(80, -1),
                                     0, nullptr, wxCB_READONLY);
        engineCombo->Append(U8("libqrencode"));
        engineCombo->SetSelection(0);
        topSizer->Add(engineCombo, 0);
        mainSizer->Add(topSizer, 0, wxALL, 8);

        // 2. 中间内容区
        wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);

        // 左侧：文本输入
        leftSizer = new wxStaticBoxSizer(wxVERTICAL, mainPanel, U8("文本输入"));
        textArea = new wxTextCtrl(leftSizer->GetStaticBox(), wxID_ANY, "",
                                  wxDefaultPosition, wxSize(180, 120),
                                  wxTE_MULTILINE | wxTE_WORDWRAP);
        leftSizer->Add(textArea, 1, wxEXPAND | wxALL, 6);

        charCountLabel = new wxStaticText(leftSizer->GetStaticBox(), wxID_ANY, U8("字节数: 0"),
                                          wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
        leftSizer->Add(charCountLabel, 0, wxEXPAND | wxALL, 6);

        wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
        generateBtn = new wxButton(leftSizer->GetStaticBox(), wxID_ANY, U8("生成"),
                                   wxDefaultPosition, wxSize(65, -1));
        clearBtn = new wxButton(leftSizer->GetStaticBox(), wxID_ANY, U8("清除"),
                                wxDefaultPosition, wxSize(50, -1));
        saveBtn = new wxButton(leftSizer->GetStaticBox(), wxID_ANY, U8("保存"),
                               wxDefaultPosition, wxSize(50, -1));
        saveBtn->Disable();

        btnSizer->Add(generateBtn, 0, wxRIGHT, 4);
        btnSizer->Add(clearBtn, 0, wxRIGHT, 4);
        btnSizer->AddStretchSpacer();
        btnSizer->Add(saveBtn, 0);
        leftSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 6);

        contentSizer->Add(leftSizer, 2, wxEXPAND | wxALL, 8);

        // 右侧：二维码预览
        rightSizer = new wxStaticBoxSizer(wxVERTICAL, mainPanel, U8("二维码预览"));

        wxImage initImg(200, 200);
        initImg.SetRGB(wxRect(0, 0, 200, 200), 255, 255, 255);
        qrBitmap = new wxStaticBitmap(rightSizer->GetStaticBox(), wxID_ANY, wxBitmap(initImg),
                                      wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
        
        rightSizer->Add(qrBitmap, 1, wxALIGN_CENTER | wxALL, 6);

        contentSizer->Add(rightSizer, 3, wxEXPAND | wxALL, 8); 
        mainSizer->Add(contentSizer, 1, wxEXPAND);

        // 3. 底部状态栏
        CreateStatusBar();
        SetStatusText(U8("就绪"));

        mainPanel->SetSizer(mainSizer);

        SetSize(600, 520);
        Layout();

        // 绑定事件
        textArea->Bind(wxEVT_TEXT, &QRFrame::OnTextChanged, this);
        generateBtn->Bind(wxEVT_BUTTON, &QRFrame::OnGenerate, this);
        clearBtn->Bind(wxEVT_BUTTON, &QRFrame::OnClear, this);
        saveBtn->Bind(wxEVT_BUTTON, &QRFrame::OnSave, this);
        Bind(wxEVT_SIZE, &QRFrame::OnResize, this);

        OnTextChanged(wxCommandEvent());
    }

private:
    void OnTextChanged(wxCommandEvent&) {
        wxString text = textArea->GetValue();
        long byteCount = text.ToUTF8().length();
        const long MAX_BYTES = 2953;

        if (byteCount > MAX_BYTES) {
            charCountLabel->SetLabel(wxString::Format(U8("字节数: %ld / %ld (已超限)\n(最大约支持 %ld 个汉字)"), 
                                                      byteCount, MAX_BYTES, MAX_BYTES / 3));
            charCountLabel->SetForegroundColour(*wxRED);
            SetStatusText(wxString::Format(U8("超出容量 (%ld/%ld 字节)"), byteCount, MAX_BYTES));
        } else {
            charCountLabel->SetLabel(wxString::Format(U8("字节数: %ld / %ld\n(最大约支持 %ld 个汉字)"), 
                                                      byteCount, MAX_BYTES, MAX_BYTES / 3));
            charCountLabel->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
            wxString st = GetStatusBar()->GetStatusText();
            if (st.StartsWith(U8("超出容量")) || st == U8("就绪")) {
                SetStatusText(U8("就绪"));
            }
        }
    }

    void OnGenerate(wxCommandEvent&) {
        wxString text = textArea->GetValue().Trim().Trim(false);
        if (text.IsEmpty()) {
            wxMessageBox(U8("请输入文本！"), U8("输入为空"), wxICON_WARNING);
            return;
        }

        long byteCount = text.ToUTF8().length();
        if (byteCount > 2953) {
            wxString msg = wxString::Format(U8("文本超出二维码容量限制！\n• 当前字节数: %ld\n• 最大支持: 2953 字节（约 %ld 个中文字符）"),
                                            byteCount, 2953 / 3);
            wxMessageBox(msg, U8("输入过长"), wxICON_ERROR);
            return;
        }

        generateBtn->Disable();
        generateBtn->SetLabel(U8("生成中..."));
        wxYield();

        wxImage qrImg = GenerateQRImage(text);
        if (qrImg.IsOk()) {
            originalQrImage = qrImg;
            UpdateQrDisplay();

            int version = GetVersionFromWidth(qrImg.GetWidth());
            int moduleCount = 21 + (version - 1) * 4;
            SetStatusText(wxString::Format(U8("libqrencode 版本:%d | 像素:%dx%d | 模块:%dx%d | 字节:%ld"),
                                           version, qrImg.GetWidth(), qrImg.GetHeight(),
                                           moduleCount, moduleCount, byteCount));
            saveBtn->Enable();
        } else {
            wxMessageBox(U8("无法生成二维码"), U8("生成失败"), wxICON_ERROR);
            SetStatusText(U8("生成失败"));
        }

        generateBtn->Enable();
        generateBtn->SetLabel(U8("生成"));
    }

    void OnClear(wxCommandEvent&) {
        textArea->Clear();
        originalQrImage = wxNullImage;
        SetStatusText(U8("就绪"));
        saveBtn->Disable();
        OnTextChanged(wxCommandEvent());
        UpdateQrDisplay(); 
    }

    void OnSave(wxCommandEvent&) {
        if (!originalQrImage.IsOk()) return;

        wxFileDialog saveDlg(this, U8("保存二维码"), "", "qrcode.png",
                             U8("PNG 文件 (*.png)|*.png|JPEG 文件 (*.jpg)|*.jpg|所有文件 (*.*)|*.*"),
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveDlg.ShowModal() == wxID_OK) {
            wxBitmapType type = wxBITMAP_TYPE_PNG;
            if (saveDlg.GetFilterIndex() == 1) type = wxBITMAP_TYPE_JPEG;

            if (originalQrImage.SaveFile(saveDlg.GetPath(), type)) {
                SetStatusText(U8("已保存: ") + saveDlg.GetFilename());
            } else {
                wxMessageBox(U8("保存失败"), U8("错误"), wxICON_ERROR);
            }
        }
    }

    void OnResize(wxSizeEvent& event) {
        if (leftSizer) leftSizer->GetStaticBox()->Refresh();
        if (rightSizer) rightSizer->GetStaticBox()->Refresh();
        
        mainPanel->Layout();
        UpdateQrDisplay();
        event.Skip();
    }

    void UpdateQrDisplay() {
        wxWindow* parent = qrBitmap->GetParent();
        wxSize size = parent->GetClientSize();
        
        if (size.x <= 0 || size.y <= 0) return;

        int side = wxMin(size.x, size.y) - 20;
        if (side < 50) side = 50;

        qrBitmap->SetSize(side, side);
        qrBitmap->Move((size.x - side) / 2, (size.y - side) / 2);

        wxImage bgImg(side, side);
        bgImg.SetRGB(wxRect(0, 0, side, side), 255, 255, 255);

        if (originalQrImage.IsOk()) {
            wxImage scaledQR = originalQrImage.Scale(side, side, wxIMAGE_QUALITY_HIGH);
            bgImg.Paste(scaledQR, 0, 0); 
        }

        qrBitmap->SetBitmap(wxBitmap(bgImg));
    }

    wxImage GenerateQRImage(const wxString& text) {
        std::string utf8Text = std::string(text.ToUTF8().data());
        QRcode* qrcode = QRcode_encodeString(utf8Text.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!qrcode) return wxNullImage;

        int width = qrcode->width;
        int scale = 8;
        int border = 2;
        int imgSize = (width + border * 2) * scale;

        wxImage img(imgSize, imgSize);
        img.SetRGB(wxRect(0, 0, imgSize, imgSize), 255, 255, 255);

        unsigned char* p = img.GetData();
        int rowBytes = imgSize * 3;

        for (int y = 0; y < width; ++y) {
            for (int x = 0; x < width; ++x) {
                if (qrcode->data[y * width + x] & 1) {
                    int startX = (x + border) * scale;
                    int startY = (y + border) * scale;
                    for (int sy = 0; sy < scale; ++sy) {
                        unsigned char* row = p + (startY + sy) * rowBytes;
                        for (int sx = 0; sx < scale; ++sx) {
                            int idx = (startX + sx) * 3;
                            row[idx]     = 0;
                            row[idx + 1] = 0;
                            row[idx + 2] = 0;
                        }
                    }
                }
            }
        }
        QRcode_free(qrcode);
        return img;
    }

    int GetVersionFromWidth(int imgPixelWidth) {
        int moduleCount = imgPixelWidth / 8 - 4;
        if (moduleCount < 21) return 1;
        return (moduleCount - 21) / 4 + 1;
    }

    wxTextCtrl* textArea;
    wxStaticText* charCountLabel;
    wxButton* generateBtn;
    wxButton* clearBtn;
    wxButton* saveBtn;
    wxComboBox* engineCombo;
    wxStaticBitmap* qrBitmap;
    wxImage originalQrImage;
    wxPanel* mainPanel;
    wxStaticBoxSizer* leftSizer;
    wxStaticBoxSizer* rightSizer;
};

class QRApp : public wxApp {
public:
    bool OnInit() override {
        wxInitAllImageHandlers(); 
        QRFrame* frame = new QRFrame();
        frame->Show(true);
        SetTopWindow(frame);
        return true;
    }
};

wxIMPLEMENT_APP(QRApp);