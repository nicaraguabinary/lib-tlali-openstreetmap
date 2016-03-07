# lib-tlali-openstreetmap
======

Tlali ("tierra" en Nahuatl) es una librería en C para la carga e interpretacion de datos obtenidos desde una fuente OpenStreetMap (OSM).

#Licencia
======

LGPLv2.1, ver archivo LICENSE

#Origen
======

Publicada el 07/mar/2016.

#Especificaciones generales
======

- Sólo dos archivos (tlali-osm.h y tlali-osm.c)
- La gestión de memoria es customizable mediante las marcos TLA_MALLOC y TLA_FREE.

#Utilidad
======

Principalmente en proyectos que desean consumir, procesar y almacenar datos desde una fuente de OpenStreetMap; de forma altamente eficiente.

#Como integrar en un proyecto?
======

El diseño de tlali-osm prioriza la facilidad de integrar con otros proyectos mediante simplemente apuntar-hacia o copiar los archivos tlali-osm.h y tlali-osm.c al proyecto. Estos archivos no requieren de otras librerías.

#Ejemplos y demos
======

Este repositorio incluye proyectos demo para los siguientes entornos:

- xcode
- eclipse
- visual-studio

#Contacto
======

info@nibsa.com.ni