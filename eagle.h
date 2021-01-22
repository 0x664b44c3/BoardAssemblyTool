#ifndef EAGLE_H
#define EAGLE_H
#include "dwgElements.h"
#include <QString>
#include <QColor>
namespace Eagle {
	struct Layer {
		int number;
		QString name;
		QColor color;
		int fillStyle;
		bool visible;
		bool active;
	};
	enum GridUnit {
		uUndefined,
		uMillimeters,
		uInch,
		uMils,
		uMicron
	};

}
#endif // EAGLE_H
