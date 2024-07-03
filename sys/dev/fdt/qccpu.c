/*	$OpenBSD: qccpu.c,v 1.3 2024/05/13 01:15:50 jsg Exp $	*/
/*
 * Copyright (c) 2023 Dale Rahn <drahn@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sensors.h>

#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/fdt.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_clock.h>
#include <dev/ofw/fdt.h>

#define CPUF_ENABLE		0x000
#define CPUF_DOMAIN_STATE	0x020
#define  CPUF_DOMAIN_STATE_LVAL_M	0xff
#define  CPUF_DOMAIN_STATE_LVAL_S	0
#define CPUF_DVCS_CTRL		0x0b0
#define  CPUF_DVCS_CTRL_PER_CORE	0x1
#define CPUF_FREQ_L3_REQ(n)	(0x090+(n)*0x4)
#define  CPUF_FREQ_L3_REQ_LVAL_M	0x3f
#define  CPUF_FREQ_L3_REQ_LVAL_S	0
#define CPUF_FREQ_LUT(n)	(0x100+(n)*0x4)
#define  CPUF_FREQ_LUT_SRC_M		0x1
#define  CPUF_FREQ_LUT_SRC_S		30
#define  CPUF_FREQ_LUT_CORES_M		0x7
#define  CPUF_FREQ_LUT_CORES_S		16
#define  CPUF_FREQ_LUT_LVAL_M		0xff
#define  CPUF_FREQ_LUT_LVAL_S		0
#define CPUF_VOLT_LUT		0x200
#define  CPUF_VOLT_LUT_IDX_M		0x3f
#define  CPUF_VOLT_LUT_IDX_S		16
#define  CPUF_VOLT_LUT_VOLT_M		0xfff
#define  CPUF_VOLT_LUT_VOLT_S		0
#define CPUF_PERF_STATE(n)	(0x320+(n)*0x4)

struct cpu_freq_tbl {
	uint32_t driver_data;
	uint32_t frequency;
};

#define MAX_GROUP	4
#define MAX_LUT		40

#define XO_FREQ_HZ	19200000

struct qccpu_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh[MAX_GROUP];

	int			sc_node;
	int			sc_ngroup;

	struct clock_device	sc_cd;
	uint32_t		sc_freq[MAX_GROUP][MAX_LUT];
	int			sc_num_lut[MAX_GROUP];

	struct ksensordev       sc_sensordev;
	struct ksensor          sc_hz_sensor[MAX_GROUP];
};

#define DEVNAME(sc) (sc)->sc_dev.dv_xname

int	qccpu_match(struct device *, void *, void *);
void	qccpu_attach(struct device *, struct device *, void *);
int	qccpu_set_frequency(void *, uint32_t *, uint32_t);
uint32_t qccpu_get_frequency(void *, uint32_t *);
uint32_t qccpu_lut_to_freq(struct qccpu_softc *, int, uint32_t);
uint32_t qccpu_lut_to_cores(struct qccpu_softc *, int, uint32_t);
void	qccpu_refresh_sensor(void *arg);

void qccpu_collect_lut(struct qccpu_softc *sc, int);


const struct cfattach qccpu_ca = {
	sizeof (struct qccpu_softc), qccpu_match, qccpu_attach
};

struct cfdriver qccpu_cd = {
	NULL, "qccpu", DV_DULL
};

int
qccpu_match(struct device *parent, void *match, void *aux)
{
	struct fdt_attach_args *faa = aux;

	return OF_is_compatible(faa->fa_node, "qcom,cpufreq-epss") ||
	    OF_is_compatible(faa->fa_node, "qcom,epss-l3") ;
}

void
qccpu_attach(struct device *parent, struct device *self, void *aux)
{
	struct qccpu_softc *sc = (struct qccpu_softc *)self;
	struct fdt_attach_args *faa = aux;
	int l3;
	int i;

	sc->sc_ngroup = faa->fa_nreg;
	if (faa->fa_nreg > MAX_GROUP) {
		printf(": only %d registers supported", MAX_GROUP);
		sc->sc_ngroup = MAX_GROUP;
	}

	l3 = OF_is_compatible(faa->fa_node, "qcom,epss-l3");

	sc->sc_iot = faa->fa_iot;
	for (i = 0; i < sc->sc_ngroup; i++) {
		if (bus_space_map(sc->sc_iot, faa->fa_reg[i].addr,
		    faa->fa_reg[i].size, 0, &sc->sc_ioh[i])) {
			printf(": can't map registers (cluster %d)\n", i);
			return;
		}
	}

	sc->sc_node = faa->fa_node;

	printf("\n");

	for (i = 0; i < sc->sc_ngroup; i++)
		qccpu_collect_lut(sc, i);

	sc->sc_cd.cd_node = faa->fa_node;
	sc->sc_cd.cd_cookie = sc;
	sc->sc_cd.cd_get_frequency = qccpu_get_frequency;
	sc->sc_cd.cd_set_frequency = qccpu_set_frequency;
	clock_register(&sc->sc_cd);

	strlcpy(sc->sc_sensordev.xname, sc->sc_dev.dv_xname,
	    sizeof(sc->sc_sensordev.xname));

	for (i = 0; i < sc->sc_ngroup; i++) {
		sc->sc_hz_sensor[i].type = SENSOR_FREQ;
		sensor_attach(&sc->sc_sensordev, &sc->sc_hz_sensor[i]);
		if (l3) {
			strlcpy(sc->sc_hz_sensor[i].desc, "l3",
			    sizeof(sc->sc_hz_sensor[i].desc));

		} else {
			if (i == 0)
				snprintf(sc->sc_hz_sensor[i].desc,
				    sizeof(sc->sc_hz_sensor[i].desc),
				    "little" );
			else
				snprintf(sc->sc_hz_sensor[i].desc,
				    sizeof(sc->sc_hz_sensor[i].desc),
				    sc->sc_ngroup > 2 ?
				    "big%d" : "big", i-1 );
		}
		printf(" attaching sensor %d\n", i);
	}
	sensordev_install(&sc->sc_sensordev);
	sensor_task_register(sc, qccpu_refresh_sensor, 1);
}

void
qccpu_collect_lut(struct qccpu_softc *sc, int group)
{
	int prev_freq = 0;
	uint32_t freq;
	int idx;
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh[group];

	for (idx = 0; ; idx++) {
		freq = bus_space_read_4(iot, ioh,
		    CPUF_FREQ_LUT(idx));

		if (idx != 0 && prev_freq == freq) {
			sc->sc_num_lut[group] = idx;
			break;
		}

		sc->sc_freq[group][idx] = freq;

#ifdef DEBUG
		printf("%s: %d: %x %u\n", DEVNAME(sc), idx, freq,
		    qccpu_lut_to_freq(sc, idx, group));
#endif /* DEBUG */

		prev_freq = freq;
		if (idx >= MAX_LUT-1)
			break;
	}

	return;
}

uint32_t
qccpu_get_frequency(void *cookie, uint32_t *cells)
{
	struct qccpu_softc *sc = cookie;
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh;
	uint32_t		lval;
	uint32_t		group;

	if (cells[0] >= sc->sc_ngroup) {
		printf("%s: bad cell %d\n", __func__, cells[0]);
		return 0;
	}
	group = cells[0];

	ioh = sc->sc_ioh[cells[0]];

	lval = (bus_space_read_4(iot, ioh, CPUF_DOMAIN_STATE)
	    >> CPUF_DOMAIN_STATE_LVAL_S) & CPUF_DOMAIN_STATE_LVAL_M;
	return lval *XO_FREQ_HZ;
}

int
qccpu_set_frequency(void *cookie, uint32_t *cells, uint32_t freq)
{
	struct qccpu_softc *sc = cookie;
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh;
	int			index = 0;
	int			numcores, i;
	uint32_t		group;

	if (cells[0] >= sc->sc_ngroup) {
		printf("%s: bad cell %d\n", __func__, cells[0]);
		return 1;
	}
	group = cells[0];

	ioh = sc->sc_ioh[group];

	while (index < sc->sc_num_lut[group]) {
		if (freq == qccpu_lut_to_freq(sc, index, group))
			break;

		if (freq < qccpu_lut_to_freq(sc, index, group)) {
			/* select next slower if not match, not zero */
			if (index != 0)
				index = index - 1;
			break;
		}

		index++;
	}

#ifdef DEBUG
	printf("%s called freq %u index %d\n", __func__, freq, index);
#endif /* DEBUG */

	if ((bus_space_read_4(iot, ioh, CPUF_DVCS_CTRL) &
	    CPUF_DVCS_CTRL_PER_CORE) != 0)
		numcores = qccpu_lut_to_cores(sc, index, group);
	else
		numcores = 1;
	for (i = 0; i < numcores; i++) {
		bus_space_write_4(iot, ioh, CPUF_PERF_STATE(i), index);
		bus_space_write_4(iot, ioh, CPUF_FREQ_L3_REQ(i), index);
	}

	return 0;
}

uint32_t
qccpu_lut_to_freq(struct qccpu_softc *sc, int index, uint32_t group)
{
	return XO_FREQ_HZ *
	    ((sc->sc_freq[group][index] >> CPUF_FREQ_LUT_LVAL_S)
	     & CPUF_FREQ_LUT_LVAL_M);
}

uint32_t
qccpu_lut_to_cores(struct qccpu_softc *sc, int index, uint32_t group)
{
	return ((sc->sc_freq[group][index] >> CPUF_FREQ_LUT_CORES_S)
	    & CPUF_FREQ_LUT_CORES_M);
}

void
qccpu_refresh_sensor(void *arg)
{
        struct qccpu_softc *sc = arg;
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh;
	int		 idx;
	uint32_t	 lval;

	for (idx = 0; idx < sc->sc_ngroup; idx++) {
		ioh = sc->sc_ioh[idx];
		
		lval = (bus_space_read_4(iot, ioh, CPUF_DOMAIN_STATE)
		    >> CPUF_DOMAIN_STATE_LVAL_S) & CPUF_DOMAIN_STATE_LVAL_M;
		sc->sc_hz_sensor[idx].value = 1000000ULL * lval * XO_FREQ_HZ;
	}
}
