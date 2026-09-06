#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_message.H>
#include <FL/Fl_RGB_Image.H>
#include <qrencode.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>
#include <FL/Fl_Multiline_Output.H>
#ifdef _WIN32
#include <windows.h>
#include <FL/x.H> 
#define IDI_ICON1 101 
#endif


class QRWindow : public Fl_Window {
public:

    void show() override {
        Fl_Window::show(); 
        
    #ifdef _WIN32
        HWND hwnd = fl_xid(this); 

        HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), 
                                       IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
        
        if (hIcon) {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
    #endif
    }

    QRWindow() : Fl_Window(600, 520, "二维码生成器 by geckotao") {
        // 允许调节大小，最小 500x400
        size_range(500, 400, 0, 0);
        
        #ifdef _WIN32
        Fl::set_font(FL_HELVETICA, "Microsoft YaHei");
        #endif
        
        // 底部状态栏
        statusBar = new Fl_Box(0, 490, 600, 30, "就绪");
        statusBar->box(FL_DOWN_BOX);
        statusBar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        statusBar->labelsize(11);
        statusBar->labelcolor(FL_BLACK);
        
        // 主内容区（不含状态栏）
        mainGroup = new Fl_Group(0, 0, 600, 490);
        
        // 左侧面板
        leftGroup = new Fl_Group(8, 8, 230, 474);
        leftGroup->box(FL_THIN_UP_BOX);
        
        textBuffer = new Fl_Text_Buffer();
        textEditor = new Fl_Text_Editor(16, 16, 214, 200);
        textEditor->buffer(textBuffer);
        textEditor->wrap_mode(Fl_Text_Editor::WRAP_AT_BOUNDS, 0);
        textBuffer->add_modify_callback(bufferModifiedCallback, this);
        
        // 使用 Fl_Output 显示字节数（只读文本框）
        charCountOutput = new Fl_Multiline_Output(16, 224, 214, 48);
        charCountOutput->value("字节数: 0 / 2953\n(最大约支持 984 个汉字)");
        charCountOutput->textsize(11);
        charCountOutput->textcolor(FL_BLACK);
        charCountOutput->box(FL_FLAT_BOX);
        charCountOutput->color(FL_BACKGROUND_COLOR);
        
        generateBtn = new Fl_Button(16, 280, 70, 28, "生成");
        generateBtn->callback(generateCallback, this);
        clearBtn = new Fl_Button(92, 280, 56, 28, "清除");
        clearBtn->callback(clearCallback, this);
        saveBtn = new Fl_Button(154, 280, 76, 28, "保存");
        saveBtn->callback(saveCallback, this);
        saveBtn->deactivate();
        
        leftGroup->end();
        
        // 右侧面板
        rightGroup = new Fl_Group(246, 8, 346, 474);
        rightGroup->box(FL_THIN_UP_BOX);
        
        qrBox = new Fl_Box(254, 16, 330, 458);
        qrBox->box(FL_DOWN_BOX);
        qrBox->color(FL_WHITE);
        
        rightGroup->end();
        
        mainGroup->end();
        
        updateQrDisplay();
        textChangedCallback(nullptr, this);
        
        end();
    }
    
    ~QRWindow() {
        delete textBuffer;
        if (qrImage) delete qrImage;
        if (displayImage) delete displayImage;
    }

private:
    // 控件
    Fl_Group* mainGroup;
    Fl_Group* leftGroup;
    Fl_Group* rightGroup;
    Fl_Text_Editor* textEditor;
    Fl_Text_Buffer* textBuffer;
    Fl_Multiline_Output* charCountOutput;
    Fl_Button* generateBtn;
    Fl_Button* clearBtn;
    Fl_Button* saveBtn;
    Fl_Box* qrBox;
    Fl_Box* statusBar;
    
    // 数据
    Fl_RGB_Image* qrImage = nullptr;
    Fl_RGB_Image* displayImage = nullptr;
    std::vector<unsigned char> imageData;
    int currentImgSize = 0;
    
    // 回调函数
    static void bufferModifiedCallback(int pos, int nInserted, int nDeleted, 
                                   int nRestyled, const char* deletedText, 
                                   void* cbArg) {
    static_cast<QRWindow*>(cbArg)->updateCharCount();
    }
    static void textChangedCallback(Fl_Widget*, void* data) { 
        static_cast<QRWindow*>(data)->updateCharCount(); 
    }
    static void generateCallback(Fl_Widget*, void* data) { 
        static_cast<QRWindow*>(data)->onGenerate(); 
    }
    static void clearCallback(Fl_Widget*, void* data) { 
        static_cast<QRWindow*>(data)->onClear(); 
    }
    static void saveCallback(Fl_Widget*, void* data) { 
        static_cast<QRWindow*>(data)->onSave(); 
    }
    
    void updateCharCount() {
        const char* text = textBuffer->text();
        long byteCount = text ? strlen(text) : 0;
        const long MAX_BYTES = 2953;
        char buf[128];
        
        if (byteCount > MAX_BYTES) {
            snprintf(buf, sizeof(buf), "字节数: %ld / %ld (已超限)\n(最大约支持 %ld 个汉字)", 
                     byteCount, MAX_BYTES, MAX_BYTES / 3);
            charCountOutput->textcolor(FL_RED);
            snprintf(buf, sizeof(buf), "超出容量 (%ld/%ld 字节)", byteCount, MAX_BYTES);
            statusBar->copy_label(buf);
        } else {
            snprintf(buf, sizeof(buf), "字节数: %ld / %ld\n(最大约支持 %ld 个汉字)", 
                     byteCount, MAX_BYTES, MAX_BYTES / 3);
            charCountOutput->textcolor(FL_BLACK);
            statusBar->copy_label("就绪");
        }
        charCountOutput->value(buf);
        charCountOutput->redraw();
        statusBar->redraw();
    }
    
    void onGenerate() {
        const char* text = textBuffer->text();
        if (!text || strlen(text) == 0) { 
            fl_message("请输入文本！"); 
            return; 
        }
        
        long byteCount = strlen(text);
        if (byteCount > 2953) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                     "文本超出二维码容量限制！\n• 当前字节数: %ld\n• 最大支持: 2953 字节（约 %ld 个中文字符）",
                     byteCount, 2953 / 3);
            fl_alert("%s", msg);
            return;
        }
        
        generateBtn->deactivate();
        generateBtn->label("生成中...");
        Fl::check();
        
        QRcode* qrcode = QRcode_encodeString(text, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!qrcode) {
            fl_alert("无法生成二维码");
            statusBar->copy_label("生成失败");
            generateBtn->activate();
            generateBtn->label("生成");
            return;
        }
        
        int width = qrcode->width;
        int scale = 8;
        int border = 2;
        currentImgSize = (width + border * 2) * scale;
        imageData.assign(currentImgSize * currentImgSize * 3, 255);
        
        for (int y = 0; y < width; ++y) {
            for (int x = 0; x < width; ++x) {
                if (qrcode->data[y * width + x] & 1) {
                    int startX = (x + border) * scale;
                    int startY = (y + border) * scale;
                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            int idx = ((startY + sy) * currentImgSize + (startX + sx)) * 3;
                            imageData[idx] = 0;
                            imageData[idx + 1] = 0;
                            imageData[idx + 2] = 0;
                        }
                    }
                }
            }
        }
        
        QRcode_free(qrcode);
        
        if (qrImage) delete qrImage;
        qrImage = new Fl_RGB_Image(imageData.data(), currentImgSize, currentImgSize, 3);
        
        updateQrDisplay();
        
        int version = (currentImgSize / 8 - 4 - 21) / 4 + 1;
        if (version < 1) version = 1;
        int moduleCount = 21 + (version - 1) * 4;
        char status[128];
        snprintf(status, sizeof(status), 
                 "libqrencode 版本:%d | 像素:%dx%d | 模块:%dx%d | 字节:%ld",
                 version, currentImgSize, currentImgSize, moduleCount, moduleCount, byteCount);
        statusBar->copy_label(status);
        statusBar->redraw();
        
        saveBtn->activate();
        generateBtn->activate();
        generateBtn->label("生成");
    }
    
    void onClear() {
        textBuffer->text("");
        if (qrImage) {
            delete qrImage;
            qrImage = nullptr;
        }
        imageData.clear();
        updateQrDisplay();
        statusBar->copy_label("就绪");
        saveBtn->deactivate();
        updateCharCount();
    }
    
    void onSave() {
        if (imageData.empty()) return;
        
        Fl_Native_File_Chooser chooser;
        chooser.title("保存二维码");
        chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
        chooser.filter("PNG 文件\t*.png\nJPEG 文件\t*.jpg\n所有文件\t*.*");
        chooser.preset_file("qrcode.png");
        
        if (chooser.show() == 0) {
            const char* filename = chooser.filename();
            int filterIndex = chooser.filter_value();
            
            int result = 0;
            if (filterIndex == 0) {
                result = stbi_write_png(filename, currentImgSize, currentImgSize, 3, imageData.data(), 0);
            } else if (filterIndex == 1) {
                result = stbi_write_jpg(filename, currentImgSize, currentImgSize, 3, imageData.data(), 90);
            }
            
            if (result) {
                std::string path(filename);
                std::string fname = path.substr(path.find_last_of("/\\") + 1);
                char status[256];
                snprintf(status, sizeof(status), "已保存: %s", fname.c_str());
                statusBar->copy_label(status);
            } else {
                fl_alert("保存失败");
            }
        }
    }
    
    void updateQrDisplay() {
        if (!qrImage) {
            qrBox->image(nullptr);
            qrBox->redraw();
            return;
        }
        
        int boxW = qrBox->w() - 20;
        int boxH = qrBox->h() - 20;
        int side = boxW < boxH ? boxW : boxH;
        if (side < 50) side = 50;
        
        if (displayImage) delete displayImage;
        displayImage = (Fl_RGB_Image*)qrImage->copy(side, side);
        
        qrBox->image(displayImage);
        qrBox->redraw();
    }
    
    // 窗口大小改变时重新布局所有控件
    void resize(int x, int y, int w, int h) override {
        Fl_Window::resize(x, y, w, h);
        
        int statusH = 30;
        int margin = 8;
        int contentH = h - statusH - margin * 2;
        int leftW = 230;
        int rightW = w - leftW - margin * 3;
        
        // 调整状态栏
        statusBar->resize(0, h - statusH, w, statusH);
        
        // 调整主内容区
        mainGroup->resize(0, 0, w, h - statusH);
        
        // 调整左侧面板
        leftGroup->resize(margin, margin, leftW, contentH);

        // 重新分配垂直空间：文本框占大部分，字节数和按钮紧凑排列在底部
        int textH = contentH - 110;        // 文本框高度 = 总高度 - 底部预留空间
        int charY = margin + textH + 16;   // 字节数显示区的 Y 坐标
        int btnY  = charY + 56;            // 按钮行的 Y 坐标

        textEditor->resize(margin + 8, margin + 8, leftW - 16, textH);
        charCountOutput->resize(margin + 8, charY, leftW - 16, 48);
        generateBtn->resize(margin + 8, btnY, 70, 28);
        clearBtn->resize(margin + 84, btnY, 56, 28);
        saveBtn->resize(margin + 146, btnY, leftW - 154, 28);
        
        // 调整右侧面板
        rightGroup->resize(margin + leftW + margin, margin, rightW, contentH);
        qrBox->resize(margin + leftW + margin + 8, margin + 8, rightW - 16, contentH - 16);
        
        updateQrDisplay();
        redraw();
    }
};

int main(int argc, char** argv) {

    // 如果你想要纯白背景，可以把 240 都改成 255
    Fl::background(240, 240, 240); 
    
    // 2. 将文本输入框等控件的背景色保持为纯白
    Fl::background2(255, 255, 255); 

    QRWindow window;
    window.show();
    return Fl::run();
}