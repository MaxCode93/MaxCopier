#include "bandejatarea.h"

#include <QWidget>

#ifdef Q_OS_WIN
#include <QImage>
#include <QPainter>
#include <QPixmap>

#include <shobjidl.h>
#include <windows.h>
#endif

namespace maxcopier {

#ifdef Q_OS_WIN
namespace {

/// Icono de pausa sobre fondo transparente para la superposición del botón.
HICON crearIconoDePausa()
{
    QPixmap lienzo(16, 16);
    lienzo.fill(Qt::transparent);
    QPainter pintor(&lienzo);
    pintor.setRenderHint(QPainter::Antialiasing);
    pintor.setPen(Qt::NoPen);
    pintor.setBrush(QColor(Qt::white));
    pintor.drawRoundedRect(QRectF(2.0, 2.0, 4.5, 12.0), 2.0, 2.0);
    pintor.drawRoundedRect(QRectF(9.5, 2.0, 4.5, 12.0), 2.0, 2.0);
    pintor.end();
    return lienzo.toImage().toHICON();
}

} // namespace

class BandejaDeTarea::Impl {
public:
    Impl()
    {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
            IID_ITaskbarList3, reinterpret_cast<void **>(&m_lista));
    }

    ~Impl()
    {
        if (m_iconoPausa)
            DestroyIcon(m_iconoPausa);
        if (m_lista)
            m_lista->Release();
        CoUninitialize();
    }

    ITaskbarList3 *m_lista = nullptr;
    HICON m_iconoPausa = nullptr;
    HWND m_ventana = nullptr;
};

#else

class BandejaDeTarea::Impl {
public:
    ~Impl() = default;
};

#endif

BandejaDeTarea::BandejaDeTarea()
    : m_impl(new Impl)
{
}

BandejaDeTarea::~BandejaDeTarea()
{
    delete m_impl;
}

void BandejaDeTarea::fijar(
    QWidget *ventana, qint64 hecho, qint64 total, bool pausada, bool activa)
{
#ifdef Q_OS_WIN
    if (!m_impl || !m_impl->m_lista || !ventana)
        return;

    const HWND manejador = reinterpret_cast<HWND>(ventana->winId());
    if (manejador != m_impl->m_ventana) {
        m_impl->m_ventana = manejador;
        m_impl->m_lista->SetProgressState(manejador, TBPF_NOPROGRESS);
    }

    if (activa && total > 0) {
        m_impl->m_lista->SetProgressState(manejador,
            pausada ? TBPF_PAUSED : TBPF_NORMAL);
        m_impl->m_lista->SetProgressValue(manejador,
            static_cast<ULONGLONG>(qBound<qint64>(0, hecho, total)),
            static_cast<ULONGLONG>(total));
    } else {
        m_impl->m_lista->SetProgressState(manejador, TBPF_NOPROGRESS);
    }

    if (pausada) {
        if (!m_impl->m_iconoPausa)
            m_impl->m_iconoPausa = crearIconoDePausa();
        m_impl->m_lista->SetOverlayIcon(manejador, m_impl->m_iconoPausa, L"pausa");
    } else {
        m_impl->m_lista->SetOverlayIcon(manejador, nullptr, nullptr);
    }
#else
    Q_UNUSED(ventana)
    Q_UNUSED(hecho)
    Q_UNUSED(total)
    Q_UNUSED(pausada)
    Q_UNUSED(activa)
#endif
}

} // namespace maxcopier
