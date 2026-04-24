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
			<view class="dev-cart">
				<view class="">
					<view class="dev-name">风扇</view>
					<image class="dev-logo" src="../../static/fan.png" mode="">
					</image>
				</view>
				<view class="fan-controls">
					<button class="fan-btn" :class="{ active: fanSpeed === 0 }" @click="setFanSpeed(0)">
						停止
					</button>
					<button class="fan-btn" :class="{ active: fanSpeed === 1 }" @click="setFanSpeed(1)">
						一档
					</button>
					<button class="fan-btn" :class="{ active: fanSpeed === 2 }" @click="setFanSpeed(2)">
						二档
					</button>
					<button class="fan-btn" :class="{ active: fanSpeed === 3 }" @click="setFanSpeed(3)">
						三档
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
	export default {
		data() {
			return {
				temp: '',
				humi: '',
				led: false,
				fanSpeed: 0,
				token: '',
			}
		},
		onLoad() {
			const params = {
				author_key: 'qFh4YyMWS3+z3YYzdznRgOeaUB52VTGVxIa0paWZLQEFf45iaAHZZAiJBAdH/FK8',
				version: '2022-05-01',
				user_id: '461812',
			}
			this.token = createCommonToken(params);
		},
		onShow() {
			this.fetchDevData();
			setInterval(() => {
				this.fetchDevData();
			}, 3000)
		},
		methods: {
			findPropValue(list, identifier) {
				if (!Array.isArray(list)) return undefined;
				const item = list.find(it => it && it.identifier === identifier);
				return item ? item.value : undefined;
			},
			fetchDevData() {
				uni.request({
					url: 'https://iot-api.heclouds.com/thingmodel/query-device-property',
					method: 'GET',

					data: {
						product_id: '1b8L52evN5',
						device_name: 'dev01',
					},
					header: {
						'authorization': this.token //自定义请求头信息
					},
					success: (res) => {
						console.log(res.data);
						const data = res && res.data && res.data.data ? res.data.data : [];
						const fanVal = this.findPropValue(data, 'fan_value');
						const humiVal = this.findPropValue(data, 'humidity_value');
						const ledVal = this.findPropValue(data, 'led');
						const tempVal = this.findPropValue(data, 'temp_value');

						if (fanVal !== undefined) this.fanSpeed = parseInt(fanVal) || 0;
						if (humiVal !== undefined) this.humi = humiVal;
						if (ledVal !== undefined) this.led = (ledVal === true || ledVal === 'true' || ledVal === 1 || ledVal === '1');
						if (tempVal !== undefined) this.temp = tempVal;
					}
				});
			},
			onLedSwitch(event) {
				console.log(event.detail.value);
				let value = event.detail.value;
				uni.request({
					url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
					method: 'POST',

					data: {
						product_id: '1b8L52evN5',
						device_name: 'dev01',
						params: {
							"led": value
						}
					},
					header: {
						'authorization': this.token //自定义请求头信息
					},
					success: () => {
						console.log('LED ' + (value ? 'ON' : 'OFF') + ' !');
					}
				});
			},
			setFanSpeed(speed) {
				this.fanSpeed = speed;
				uni.request({
					url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
					method: 'POST',
					data: {
						product_id: '1b8L52evN5',
						device_name: 'dev01',
						params: {
							"fan_value": speed
						}
					},
					header: {
						'authorization': this.token
					},
					success: () => {
						console.log('风扇已设置为' + (
							speed === 0 ? '停止' :
							speed === 1 ? '一档' :
							speed === 2 ? '二档' : '三档'
						));
					}
				});
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

	.fan-controls {
		display: grid; /* 使用网格布局 */
		grid-template-columns: 1fr 1fr; /* 两列等宽 */
		grid-gap: 8rpx; /* 按钮之间的间距 */
		padding: 10rpx;
	}

	.fan-btn {
		width: 80rpx;
		height: 60rpx;
		line-height: 30rpx;
		font-size: 18rpx;
		padding: 0;
		background-color: #f5f5f5;
		color: #6d6d6d;
		display: flex;
		justify-content: center;
		align-items: center;
	}

	.fan-btn.active {
		background-color: #2b9939;
		color: white;
	}

	.title {
		font-size: 36rpx;
		color: #8f8f94;
	}
</style>