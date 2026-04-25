<template>
	<view class="wrap">
		<view class="dev-area">
			<view class="dev-cart">
				<view class="">
					<view class="dev-name">温度</view>
					<image class="dev-logo" src="../../static/temp.png" mode="">
					</image>
				</view>
				<view class="dev-data">{{temp}}℃</view>
			</view>
			<view class="dev-cart">
				<view class="">
					<view class="dev-name">湿度</view>
					<image class="dev-logo" src="../../static/humi.png" mode="">
					</image>
				</view>
				<view class="dev-data">{{humi}}%</view>
			</view>
			<view class="dev-cart">
				<view class="">
					<view class="dev-name">台灯</view>
					<image class="dev-logo" src="../../static/led.png" mode="">
					</image>
				</view>
				<switch :checked="led" @change="onLedSwitch" color="#2b9939" />
			</view>
			<view class="dev-cart fan-card">
				<view class="fan-head">
					<view class="dev-name">风扇</view>
					<image class="dev-logo" src="../../static/fan.png" mode="">
					</image>
					<view class="fan-state">当前档位 {{fanSpeed}}</view>
				</view>
				<view class="fan-controls">
					<button class="fan-btn" :class="{ active: fanSpeed === 0 }" @click="setFanSpeed(0)">
						停止
					</button>
					<button class="fan-btn" :class="{ active: fanSpeed === 1 }" @click="setFanSpeed(1)">
						1
					</button>
					<button class="fan-btn" :class="{ active: fanSpeed === 2 }" @click="setFanSpeed(2)">
						2
					</button>
					<button class="fan-btn" :class="{ active: fanSpeed === 3 }" @click="setFanSpeed(3)">
						3
					</button>
					<button class="fan-btn" :class="{ active: fanSpeed === 4 }" @click="setFanSpeed(4)">
						4
					</button>
					<button class="fan-btn" :class="{ active: fanSpeed === 5 }" @click="setFanSpeed(5)">
						5
					</button>
				</view>
			</view>
		</view>
	</view>

</template>

<script>
	const {
		createCommonToken
	} = require('@/key.js')

	const ONENET_CONFIG = {
		productId: 'd203p9ta5l',
		deviceName: 'Device',
		queryUrl: 'https://iot-api.heclouds.com/thingmodel/query-device-property',
		setUrl: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
		property: {
			temp: 'temp_value',
			humi: 'humidity_value',
			led: 'led',
			fan: 'fan_value'
		}
	}
	export default {
		data() {
			return {
				temp: '',
				humi: '',
				led: false,
				fanSpeed: 0,
				token: '',
				pollTimer: null,
				isFetching: false,
			}
		},
		onLoad() {
			const params = {
				author_key: 'qdeBI02D6akDEWdrlxPNUTGXUSVpsyHFcTPJE0cjEXs=',
				version: '2022-05-01',
				res: 'products/' + ONENET_CONFIG.productId,
			}
			this.token = createCommonToken(params);
		},
		onShow() {
			this.startPolling();
		},
		onHide() {
			this.stopPolling();
		},
		onUnload() {
			this.stopPolling();
		},
		methods: {
			startPolling() {
				if (this.pollTimer != null) {
					return;
				}

				this.fetchDevData();
				this.pollTimer = setInterval(() => {
					this.fetchDevData();
				}, 3000);
			},
			stopPolling() {
				if (this.pollTimer != null) {
					clearInterval(this.pollTimer);
					this.pollTimer = null;
				}
			},
			findPropValue(list, identifier) {
				if (!Array.isArray(list)) return undefined;
				const item = list.find(it => it && it.identifier === identifier);
				return item ? item.value : undefined;
			},
			fetchDevData() {
				if (this.isFetching) {
					return;
				}

				this.isFetching = true;
				uni.request({
					url: ONENET_CONFIG.queryUrl,
					method: 'GET',

					data: {
						product_id: ONENET_CONFIG.productId,
						device_name: ONENET_CONFIG.deviceName,
					},
					header: {
						'authorization': this.token //自定义请求头信息
					},
					success: (res) => {
						console.log(res.data);
						const resp = res && res.data ? res.data : {};
						if (resp.code && resp.code !== 0) {
							if (resp.code === 10403) {
								this.stopPolling();
								uni.showToast({
									icon: 'none',
									title: '设备无权限(10403)'
								});
								console.error('OneNET 10403: token资源或账号无该设备权限', {
									product_id: ONENET_CONFIG.productId,
									device_name: ONENET_CONFIG.deviceName,
									msg: resp.msg,
									request_id: resp.request_id
								});
							}
							return;
						}
						const data = resp.data || [];
						const fanVal = this.findPropValue(data, ONENET_CONFIG.property.fan);
						const humiVal = this.findPropValue(data, ONENET_CONFIG.property.humi);
						const ledVal = this.findPropValue(data, ONENET_CONFIG.property.led);
						const tempVal = this.findPropValue(data, ONENET_CONFIG.property.temp);

						if (fanVal !== undefined) this.fanSpeed = parseInt(fanVal) || 0;
						if (humiVal !== undefined) this.humi = humiVal;
						if (ledVal !== undefined) this.led = (ledVal === true || ledVal === 'true' || ledVal === 1 || ledVal === '1');
						if (tempVal !== undefined) this.temp = tempVal;
					},
					complete: () => {
						this.isFetching = false;
					}
				});
			},
			pushDeviceProperty(params, onSuccessMessage) {
				uni.request({
					url: ONENET_CONFIG.setUrl,
					method: 'POST',
					data: {
						product_id: ONENET_CONFIG.productId,
						device_name: ONENET_CONFIG.deviceName,
						params: params
					},
					header: {
						'authorization': this.token
					},
					success: (res) => {
						const resp = res && res.data ? res.data : {};
						if (resp.code && resp.code !== 0) {
							if (resp.code === 10403) {
								uni.showToast({
									icon: 'none',
									title: '无控制权限(10403)'
								});
								console.error('OneNET 10403: 下发权限不足', {
									product_id: ONENET_CONFIG.productId,
									device_name: ONENET_CONFIG.deviceName,
									msg: resp.msg,
									request_id: resp.request_id
								});
							} else {
								uni.showToast({
									icon: 'none',
									title: '下发失败 code=' + resp.code
								});
							}
							return;
						}

						if (onSuccessMessage) {
							console.log(onSuccessMessage);
						}
					},
					fail: () => {
						uni.showToast({
							icon: 'none',
							title: '云端下发失败'
						});
					}
				});
			},
			onLedSwitch(event) {
				console.log(event.detail.value);
				const value = event.detail.value;
				this.led = value;
				this.pushDeviceProperty({
					led: value
				}, 'LED ' + (value ? 'ON' : 'OFF') + ' !');
			},
			setFanSpeed(speed) {
				this.fanSpeed = speed;
				this.pushDeviceProperty({
					fan_value: speed
				}, '风扇已设置为' + (
					speed === 0 ? '停止' :
						speed + '档'
				));
			}
		}
	}
</script>

<style>
	.wrap {
		padding: 30rpx;
	}

	.dev-area {
		display: flex;
		justify-content: space-between;
		flex-wrap: wrap;
	}

	.dev-cart {
		height: 150rpx;
		width: 320rpx;
		border-radius: 30rpx;
		margin-top: 30rpx;
		display: flex;
		justify-content: space-around;
		align-items: center;
		box-shadow: 0 0 15rpx #ccc;
	}

	.dev-name {
		font-size: 20rpx;
		text-align: center;
		color: #6d6d6d;

	}

	.dev-logo {
		width: 70rpx;
		height: 70rpx;
		margin-top: 10rpx;
	}

	.dev-data {
		font-size: 50rpx;
		color: #6d6d6d;
	}

	.fan-card {
		width: 100%;
		height: auto;
		padding: 20rpx 22rpx;
		justify-content: space-between;
		align-items: stretch;
		background: linear-gradient(145deg, #ffffff 0%, #f4f8f6 100%);
	}

	.fan-head {
		width: 108rpx;
		display: flex;
		flex-direction: column;
		align-items: center;
		gap: 10rpx;
	}

	.fan-controls {
		flex: 1;
		display: grid;
		grid-template-columns: repeat(3, minmax(0, 1fr));
		grid-template-rows: repeat(2, 1fr);
		grid-gap: 16rpx;
		padding-left: 14rpx;
		align-content: stretch;
	}

	.fan-btn {
		width: 100%;
		height: 80rpx;
		line-height: 80rpx;
		font-size: 30rpx;
		font-weight: 600;
		padding: 0;
		background: #edf2ef;
		color: #4a5a52;
		display: flex;
		justify-content: center;
		align-items: center;
		border: 2rpx solid #dbe6df;
		border-radius: 22rpx;
		box-shadow: 0 6rpx 12rpx rgba(28, 64, 40, 0.08);
	}

	.fan-btn.active {
		background: linear-gradient(135deg, #2b9939 0%, #43c96a 100%);
		color: white;
		border-color: #2b9939;
		box-shadow: 0 10rpx 18rpx rgba(43, 153, 57, 0.28);
	}

	.fan-state {
		min-width: 110rpx;
		height: 38rpx;
		line-height: 38rpx;
		text-align: center;
		font-size: 20rpx;
		color: #1f6b2a;
		background: #e5f5e8;
		border-radius: 999rpx;
		padding: 0 12rpx;
	}

	.title {
		font-size: 36rpx;
		color: #8f8f94;
	}
</style>